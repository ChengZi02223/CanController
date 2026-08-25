#include "CanController.h"
#include "SettingPage.h"
#include "CalibrationPage.h"
#include "CanConfig.h"
#include "CanDriver.h"
#include "can_cmd.h"
#include "BasicInfoBar.h"
#include "Utils.h"

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
    connect(setting_page_, &SettingPage::SendInfoChanged, calibration_page_, &CalibrationPage::SendInfoChanged);
    InitData();
}

void CanController::InitData() {
    InitBasicInfo();
}

void CanController::InitBasicInfo() {
    can_frame frame;
    bool res = CanDriver::GetInstance()->ExecCmd(SDO_COB_ID, SDO_READ_TYPE_CMD, frame, 200);
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
    res = CanDriver::GetInstance()->ExecCmd(SDO_COB_ID, SDO_READ_PROTOCOL_CMD, frame, 200);
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
    res = CanDriver::GetInstance()->ExecCmd(SDO_COB_ID, SDO_READ_MANUFACTURER_ID_CMD, frame, 200);
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
