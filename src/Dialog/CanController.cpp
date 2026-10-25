#include "CanController.h"
#include "SettingPage.h"
#include "CalibrationPage.h"
#include "CanConfig.h"
#include "CanDriver.h"
#include "can_cmd.h"
#include "BasicInfoBar.h"
#include "Utils.h"
#include "CanManager.h"

#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QDebug>
#include <iostream>

CanController::CanController(QWidget* parent)
    : QWidget(parent) {
    InitWindow();
    QScreen* primaryScreen = QGuiApplication::primaryScreen();

    // 屏幕完整宽高（像素，包含任务栏）
    QRect fullRect = primaryScreen->geometry();
    int screenW = fullRect.width();
    int screenH = fullRect.height();
    setMinimumSize(screenW * 1 / 2, screenH * 3 / 4);
    setWindowIcon(QIcon(":/icons/HZLK.png"));
}

CanController::~CanController() {}

void CanController::InitWindow() {
    main_layout_ = new QVBoxLayout(this);
    CreateTabButtons();
    stacked_widget_ = new QStackedWidget(this);
    setting_page_ = new SettingPage(this);
    calibration_page_ = new CalibrationPage(this);
    stacked_widget_->addWidget(setting_page_);
    stacked_widget_->addWidget(calibration_page_);
    
    main_layout_->addWidget(stacked_widget_);

    connect(setting_page_, &SettingPage::SendInputMode, [this](InputMode mode){
        if(mode == kAuto) {
            stacked_widget_->setCurrentIndex(1);
        }
    });
    // connect(setting_page_, &SettingPage::SendInfoChanged, calibration_page_, &CalibrationPage::SendInfoChanged);
    connect(setting_page_, &SettingPage::SendInfoChanged, calibration_page_,[this](InfoType type, QString value){
        if(type == InfoType::kChannelCount) {
            calibration_page_->SetChannelCount(value);
        }
        emit calibration_page_->SendInfoChanged(type, value);
    });
    connect(setting_page_, &SettingPage::SendRampRadio, calibration_page_,  &CalibrationPage::SetRampRadio);
    connect(calibration_page_, &CalibrationPage::SendRowValue, setting_page_, &SettingPage::SendRowValue);
    connect(CanManager::GetInstance(), &CanManager::warningFrameReceived,  this, &CanController::OnWarningFrame, Qt::QueuedConnection);
    connect(CanManager::GetInstance(), &CanManager::SendWarningMessage,  this, &CanController::OnWarningMessage, Qt::QueuedConnection);
    connect(this, &CanController::SendCloseControl, calibration_page_, &CalibrationPage::OnCloseControl);
    InitData();
}

void CanController::OnWarningMessage(const QString &msg) {
    QMessageBox::warning(nullptr, tr("Warnning"), msg);
}

void CanController::OnWarningFrame(const QByteArray &data) {
    if (data.size() < 8) {
        qWarning() << "[CAN] 报警帧长度不足 8 字节:" << data.size();
        return;
    }
    const uint8_t *d = reinterpret_cast<const uint8_t*>(data.constData());

    // ================= 1. 按协议解析 =================
    const quint16 faultCode  = quint16(d[0]) | quint16(d[1]) << 8;  // Byte0~1 故障码(小端)
    const quint8  errorReg   = d[2];                                // Byte2 0x1001 当前值
    const quint8  channel    = d[3];                                // Byte3 通道/对象号
    const quint16 eventCount = quint16(d[4]) | quint16(d[5]) << 8;  // Byte4~5 事件计数
    const quint8  severity   = d[6];                                // Byte6 严重度/状态
    const quint8  levelBits  = severity & 0x03;
    const bool    stillActive = (severity & 0x04) != 0;
    const bool    derating    = (severity & 0x08) != 0;

    // ================= 2. 故障复位帧 =================
    if (faultCode == 0x0000) {
        qDebug() << "[CAN] 故障复位通知 (对应 0x6040 控制字 Bit7 上升沿)";
        return;
    }

    // ================= 3. 协议自校验 (联调抓错用) =================
    const quint8 expectBit = classToRegBit(faultCode);
    if (expectBit && (errorReg & expectBit) == 0) {
        qWarning() << "[CAN] 协议校验异常: 故障码" << hex16(faultCode)
                   << "属于" << faultClassText(faultCode)
                   << ", 但 0x1001 =" << hex8(errorReg) << "对应位未置位";
    }
    if ((errorReg & 0x01) == 0) {
        qWarning() << "[CAN] 协议校验异常: 存在激活故障但 0x1001 Bit0(通用错误)未置位";
    }

    // ================= 4. 字段翻译 =================
    QString channelText;
    switch (channel) {
    case 0x00:  channelText = QStringLiteral("整机");                  break;
    case 0x01:  channelText = QStringLiteral("TLE92464ED 驱动通道 1"); break;
    case 0x02:  channelText = QStringLiteral("TLE92464ED 驱动通道 2"); break;
    case 0x03:  channelText = QStringLiteral("TLE92464ED 驱动通道 3"); break;
    case 0x04:  channelText = QStringLiteral("TLE92464ED 驱动通道 4"); break;
    case 0x05:  channelText = QStringLiteral("PWM 通道 1");            break;
    case 0x06:  channelText = QStringLiteral("PWM 通道 2");            break;
    case 0x10:  channelText = QStringLiteral("LVDT 位移传感器");       break;
    case 0x20:  channelText = QStringLiteral("NTC 温度传感器");        break;
    case 0x30:  channelText = QStringLiteral("主电源");                break;
    case 0x40:  channelText = QStringLiteral("CAN 通信");              break;
    default:    channelText = QStringLiteral("未知通道");              break;
    }

    // 故障码具体描述: 查表, 查不到就显示"大类 + 未知具体码"
    QString faultText = faultCodeText(faultCode);
    if (faultText.isEmpty())
        faultText = faultClassText(faultCode) + QStringLiteral("未知故障码");

    // ★ 等级命名按 9.9.4: A/B = 严重故障, C/D = 轻微故障/警告
    static const QString kLevelNames[4] = {
        QStringLiteral("1级(A) 严重故障"),
        QStringLiteral("2级(B) 严重故障"),
        QStringLiteral("3级(C) 轻微故障"),
        QStringLiteral("4级(D) 轻微警告"),
    };
    const QString levelText = kLevelNames[levelBits];
    if(levelBits < 2) {
        emit SendCloseControl();
    }
    // ================= 5. 图标映射 (A/B 严重 → 红叉, C/D 轻微 → 黄/蓝) =================
    QMessageBox::Icon icon;
    switch (levelBits) {
    case 0:                                  // 1级(A) 严重
    case 1:  icon = QMessageBox::Critical; break;  // 2级(B) 严重
    case 2:  icon = QMessageBox::Warning;  break;  // 3级(C) 轻微
    default: icon = QMessageBox::Information; break; // 4级(D) 提示
    }

    // ================= 6. 拼装并弹窗 =================
    const QString title = QStringLiteral("[%1] %2 — %3")
                              .arg(levelText, channelText, faultText);

    const QString body = QStringLiteral(
                             "故障码      : %1 (%2)\n"
                             "故障描述    : %3\n"
                             "影响通道    : %4 (%5)\n"
                             "错误寄存器  : %6\n"
                             "  └ %7\n"
                             "累计发生    : %8 次\n"
                             "当前状态    : %9%10")
                             .arg(hex16(faultCode), faultClassText(faultCode))
                             .arg(faultText)
                             .arg(channelText, hex8(channel))
                             .arg(hex8(errorReg))
                             .arg(errorRegisterBits(errorReg).join(QStringLiteral("; ")))
                             .arg(eventCount)
                             .arg(stillActive ? QStringLiteral("故障激活") : QStringLiteral("已恢复"))
                             .arg(derating    ? QStringLiteral(", 降额运行中") : QString());

    QMessageBox *box = new QMessageBox(icon, title, body,
                                       QMessageBox::Ok, this,
                                       Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    box->setAttribute(Qt::WA_DeleteOnClose);
    box->setModal(false);
    box->show();
}

void CanController::InitData() {
    InitBasicInfo();
}

void CanController::SetChannelCount(const QString& channel_count) {
    calibration_page_->SetChannelCount(channel_count);
}

void CanController::InitBasicInfo() {
    can_frame frame;
    bool res = CanDriver::GetInstance()->ExecCmd(GetSDOCobId(), SDO_READ_TYPE_CMD, frame, 200);
    //设备（型号）
    BasicInfo info;
    if(!res) {
        std::cout << "Failed to read device type!" << std::endl;
        return;
    }
    auto device_type = ExtractFromDataList(frame.data, 5, 4);
    if(device_type == 0x190) {
        std::cout << "Device type: 液压设备" << std::endl;
        info.model = "液压设备";
    } else {
        info.model = "未知设备";
        std::cout << "Unknown device type: " << std::hex << device_type << std::dec << std::endl;
    }
    // 协议
    frame = {};
    res = CanDriver::GetInstance()->ExecCmd(GetSDOCobId(), SDO_READ_PROTOCOL_CMD, frame, 200);
    if (!res) {
        std::cout << "Failed to read protocol type!" << std::endl;
        return;
    }
    auto res_code = ExtractFromDataList(frame.data, 2, 1);
    if(res_code == 0x100E) {
        auto protocol_type = ExtractFromDataList(frame.data, 5, 4);
        std::cout << "Protocol type: " << std::hex << protocol_type << std::dec << std::endl;
        if(protocol_type == 0) {
            std::cout << "Protocol: CANOpen" << std::endl;
            info.protocol = "CANOpen";
        } else if(protocol_type == 1) {
            info.protocol = "J1939";
            std::cout << "Protocol: J1939" << std::endl;
        } else {
            info.protocol = "none";
            std::cout << "Unknown protocol type: " << std::hex << protocol_type << std::dec << std::endl;
        }
    }
    // 厂商
    frame = {};
    res = CanDriver::GetInstance()->ExecCmd(GetSDOCobId(), SDO_READ_MANUFACTURER_ID_CMD, frame, 200);
    if (!res) {
        std::cout << "Failed to read manufacturer ID!" << std::endl;
        return;
    }
    auto manufacturer_id = ExtractFromDataList(frame.data, 5, 4);
    std::cout << "Manufacturer ID: " << std::hex << manufacturer_id << std::dec << std::endl;
    info.manufacturer = QString::number(manufacturer_id, 16);

    setting_page_->InitBasicInfo(info);
    calibration_page_->InitBasicInfo(info);
}

void CanController::CreateTabButtons() {
    auto setting_page_btn = new QPushButton("参数配置状态", this);
    setting_page_btn->setObjectName("SettingPageTabBtn");
    setting_page_btn->setCheckable(true);
    setting_page_btn->setChecked(true);
    auto test_page_btn = new QPushButton("测试标定状态", this);
    test_page_btn->setObjectName("TestPageTabBtn");
    test_page_btn->setCheckable(true);
    test_page_btn->setChecked(false);

    auto can_config_btn = new QPushButton(this);
    can_config_btn->setFixedSize(30, 30);
    can_config_btn->setObjectName("CanCongfigBtn");
    const QString res_path = ":/icons/config.ico";
    QFile file(res_path);
    // qDebug() << "资源路径：" << res_path;
    // qDebug() << "资源是否存在：" << file.exists();
    if (file.exists())
    {
        can_config_btn->setIcon(QIcon(res_path));
        can_config_btn->setIconSize(QSize(24,24));
    }

    auto btn_layout = new QHBoxLayout();
    btn_layout->addWidget(setting_page_btn);
    btn_layout->addWidget(can_config_btn);
    btn_layout->addWidget(test_page_btn);
    main_layout_->addLayout(btn_layout);

    auto btn_group = new QButtonGroup(this);
    btn_group->addButton(setting_page_btn);
    btn_group->addButton(test_page_btn);
    btn_group->setExclusive(true); // 开启互斥

    // 2. 点击事件只需要处理页面切换，不用手动取消选中
    connect(setting_page_btn, &QPushButton::clicked, this, [this]() {
        stacked_widget_->setCurrentIndex(0);
    });

    connect(test_page_btn, &QPushButton::clicked, this, [this]() {
        stacked_widget_->setCurrentIndex(1);
    });

    connect(can_config_btn, &QPushButton::clicked, this, [this]() {
        // 打开 CAN 配置窗口
        CanConfigWin::GetInstance()->show();
    });
}
