#include "CanConfig.h"
#include "QtWidgets.h"
#include "CanController.h"
#include "CO_CMD.h"
#include "CmdManager.h"
#include "Utils.h"
#include "can_cmd.h"

#include <iomanip>
#include <sstream>
#include <unordered_map>

CanConfigWin::CanConfigWin(QWidget *parent)
    : QMainWindow(parent), canReady(false)
{
    setupUI();
    receiveTimer = new QTimer(this);
    connect(receiveTimer, &QTimer::timeout, this, &CanConfigWin::onReceiveTimer);
    receiveTimer->start(50);
    onRefreshDevices();
}

CanConfigWin::~CanConfigWin()
{
    if (canReady) onRelease();
}

void CanConfigWin::setupUI()
{
    setWindowTitle("CAN Controller - PCAN Style");
    resize(800, 600);

    QTabWidget *tabWidget = new QTabWidget(this);
    setCentralWidget(tabWidget);

    // ---------- Config Tab ----------
    QWidget *configTab = new QWidget;
    QVBoxLayout *configLayout = new QVBoxLayout(configTab);

    QGroupBox *connGroup = new QGroupBox("Connection");
    connGroup->setFixedHeight(100);
    QHBoxLayout *connLayout = new QHBoxLayout(connGroup);
    cbDevice = new QComboBox;
    btnRefresh = new QPushButton("Refresh");
    btnRefresh->setObjectName("RefreshBtn");
    btnRefresh->setFixedSize(80, 30);

    cbBaudrate = new QComboBox;
    cbBaudrate->addItems({"125 kbps", "250 kbps", "500 kbps", "1 Mbps"});
    cbBaudrate->setCurrentIndex(1);

    connLayout->addWidget(new QLabel("Device:"));
    connLayout->addWidget(cbDevice);
    connLayout->addWidget(btnRefresh);
    connLayout->addStretch();
    connLayout->addWidget(new QLabel("Baudrate:"), 0, Qt::AlignRight);
    connLayout->addWidget(cbBaudrate, 0, Qt::AlignRight);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnInit = new QPushButton("Initlize");
    btnInit->setObjectName("InitBtn");
    btnInit->setFixedSize(100, 30);
    btnRelease = new QPushButton("Release");
    btnRelease->setObjectName("ReleaseBtn");
    btnRelease->setFixedSize(100, 30);
    btnChange = new QPushButton("Change");
    btnChange->setObjectName("ChangeBtn");
    btnChange->setFixedSize(100, 30);
    btnController = new QPushButton("Controller");
    btnController->setObjectName("ControllerBtn");
    btnController->setFixedSize(100, 30);
    btnChange->setEnabled(false);
    btnController->setEnabled(true);
    btnRelease->setEnabled(false);
    btnLayout->addWidget(btnInit);
    btnLayout->addWidget(btnRelease);
    btnLayout->addStretch();
    btnLayout->addWidget(btnChange, 0, Qt::AlignRight);
    btnLayout->addWidget(btnController, 0, Qt::AlignRight);

    teLog = new QTextEdit;
    teLog->setReadOnly(true);

    configLayout->addWidget(connGroup);
    configLayout->addLayout(btnLayout);
    configLayout->addWidget(new QLabel("Log:"));
    configLayout->addWidget(teLog);

    // ---------- Test Tab ----------
    QWidget *testTab = new QWidget;
    QVBoxLayout *testMainLayout = new QVBoxLayout(testTab);

    // 发送区
    QGroupBox *sendGroup = new QGroupBox("Send Message");
    QVBoxLayout *sendLayout = new QVBoxLayout(sendGroup);

    QHBoxLayout *idLayout = new QHBoxLayout;
    idLayout->addWidget(new QLabel("ID (Hex):"));
    leSendId = new QLineEdit("000");
    idLayout->addWidget(leSendId);
    idLayout->addWidget(new QLabel("DLC:"));
    cbSendDlc = new QComboBox;
    for (int i = 0; i <= 8; ++i) cbSendDlc->addItem(QString::number(i));
    cbSendDlc->setCurrentIndex(8);
    idLayout->addWidget(cbSendDlc);
    idLayout->addStretch();
    sendLayout->addLayout(idLayout);

    // 合并后的数据输入框
    QHBoxLayout *dataLayout = new QHBoxLayout;
    dataLayout->addWidget(new QLabel("Data (Hex):"));
    leSendData = new QLineEdit;
    leSendData->setPlaceholderText("Enter hex bytes, e.g., 01 02 03 04 05 06 07 08");
    leSendData->setFixedHeight(30);
    dataLayout->addWidget(leSendData, 1);  // 1表示拉伸
    sendLayout->addLayout(dataLayout);

    // 添加提示标签
    QLabel *hintLabel = new QLabel("Tip: Enter hexadecimal bytes separated by spaces");
    hintLabel->setStyleSheet("color: gray; font-size: 9pt;");
    sendLayout->addWidget(hintLabel);

    btnSend = new QPushButton("Send");
    btnSend->setFixedSize(100, 30);
    btnSend->setObjectName("SendBtn");
    btnSend->setFixedWidth(150);
    btnSend->setEnabled(false);
    sendLayout->addWidget(btnSend, 1, Qt::AlignRight);
    sendLayout->addStretch();

    // 接收区
    QGroupBox *recvGroup = new QGroupBox("Received Messages");
    QVBoxLayout *recvLayout = new QVBoxLayout(recvGroup);
    twReceive = new QTableWidget(0, 5);
    twReceive->setHorizontalHeaderLabels({"Time", "ID (Hex)", "DLC", "Data", "Type"});
    twReceive->horizontalHeader()->setStretchLastSection(true);
    recvLayout->addWidget(twReceive);

    testMainLayout->addWidget(sendGroup, 1);
    testMainLayout->addWidget(recvGroup, 2);

    tabWidget->addTab(configTab, "Config");
    tabWidget->addTab(testTab, "Test");

    // 信号连接
    connect(btnRefresh, &QPushButton::clicked, this, &CanConfigWin::onRefreshDevices);
    connect(btnInit, &QPushButton::clicked, this, &CanConfigWin::onInit);
    connect(btnRelease, &QPushButton::clicked, this, &CanConfigWin::onRelease);
    connect(btnChange, &QPushButton::clicked, this, &CanConfigWin::onChangeMode);
    connect(btnController, &QPushButton::clicked, this, &CanConfigWin::onOpenController);
    connect(btnSend, &QPushButton::clicked, this, &CanConfigWin::onSend);
}

void CanConfigWin::SendAndReadSDOInfo() {}

void CanConfigWin::InitSDOInfo() {}

void CanConfigWin::onRefreshDevices()
{
    cbDevice->clear();
    channelList.clear();

    channelList = CanDriver::GetInstance()->scanAllChannels();
    if (channelList.empty()) {
        QMessageBox::information(this, "提示", "未检测到PCAN设备，请检查USB/驱动");
        return;
    }
    for (auto& info : channelList){
        QString disp = QString("%1 | %2 | %3")
            .arg(QString::fromStdString(info.channelName))
            .arg(QString::fromStdString(info.hardwareName))
            .arg(info.isAvailable ? "空闲可连接" : "已占用");
        qDebug() << "Detected channel: " << disp;
        cbDevice->addItem(QString::fromStdString(info.channelName), QString::fromStdString(info.channelName));
    }
    logMessage("Device list refreshed!");
}

void CanConfigWin::onInit()
{
    if (canReady) {
        logMessage("Already initialized, release first.", true);
        return;
    }
    QString device = cbDevice->currentData().toString();
    if (device.isEmpty()) {
        logMessage("No device selected.", true);
        return;
    }
    int baudIdx = cbBaudrate->currentIndex();
    uint32_t baudrate = 125000;
    switch (baudIdx) {
        case 0: baudrate = 125000; break;
        case 1: baudrate = 250000; break;
        case 2: baudrate = 500000; break;
        case 3: baudrate = 1000000; break;
    }
    if (CanDriver::GetInstance()->init(GetSelectedChannelHandle(), baudrate)) {
        canReady = true;
        logMessage(QString("Init OK: %1 @ %2 bps").arg(device).arg(baudrate));
        btnInit->setEnabled(false);
        btnRelease->setEnabled(true);
        btnChange->setEnabled(true);
        btnController->setEnabled(true);
        btnSend->setEnabled(true);
        updateCanStatus(true);
    } else {
        logMessage(QString("Init failed: %1").arg(device), true);
    }
}

void CanConfigWin::onRelease()
{
    if (!canReady) return;
    CanDriver::GetInstance()->close();
    canReady = false;
    logMessage("CAN released.");
    btnInit->setEnabled(true);
    btnRelease->setEnabled(false);
    btnChange->setEnabled(false);
    btnController->setEnabled(false);
    btnSend->setEnabled(false);
    updateCanStatus(false);
}

void CanConfigWin::onChangeMode() {
    if (!canReady) return;
    CanDriver::GetInstance()->ExecCmd(CHANGE_TO_CANOPEN_COB_ID, CHANGE_TO_CANOPEN_CMD, 200);
}

void CanConfigWin::onOpenController() {
    this->close();
    InitSDOInfo();
    CanController::GetInstance()->show();
}

void CanConfigWin::onSend()
{
    if (!canReady) {
        logMessage("CAN not initialized, cannot send.", true);
        return;
    }
    
    can_frame frame;
    bool ok;
    
    // 解析ID
    frame.can_id = leSendId->text().toUInt(&ok, 16);
    qDebug() << "Send Id input: " << leSendId->text() << " parsed:" << frame.can_id;
    if (!ok) {
        logMessage("Invalid ID (hex)", true);
        return;
    }
    if(frame.can_id > 0x7FF){
        frame.can_id |= CAN_EFF_FLAG;
    }

    // 获取DLC
    frame.can_dlc = cbSendDlc->currentText().toUInt();
    if (frame.can_dlc > 8) {
        logMessage("DLC cannot exceed 8", true);
        return;
    }

    // 解析数据 - 从合并的输入框中读取
    QString dataText = leSendData->text().trimmed();
    if (dataText.isEmpty()) {
        logMessage("Data input is empty!", true);
        return;
    }

    // 按空格分隔并解析十六进制
    QStringList hexList = dataText.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    
    if (hexList.size() != frame.can_dlc) {
        logMessage(QString("Data count (%1) does not match DLC (%2)").arg(hexList.size()).arg(frame.can_dlc), true);
        return;
    }

    // 解析每个字节
    for (int i = 0; i < frame.can_dlc; ++i) {
        bool hexOk;
        uint8_t byte = static_cast<uint8_t>(hexList[i].toUInt(&hexOk, 16));
        if (!hexOk) {
            logMessage(QString("Invalid hex value at position %1: %2").arg(i).arg(hexList[i]), true);
            return;
        }
        frame.data[i] = byte;
    }

    // 发送CAN帧
    if (CanDriver::GetInstance()->send(frame)) {
        QString dataStr;
        for (int i = 0; i < frame.can_dlc; ++i) {
            dataStr += QString("%1 ").arg(frame.data[i], 2, 16, QChar('0'));
        }
        logMessage(QString("Sent: ID=0x%1 DLC=%2 Data=[%3]")
                   .arg(frame.can_id & CAN_EFF_MASK, 0, 16)
                   .arg(frame.can_dlc)
                   .arg(dataStr.trimmed()));
    } else {
        logMessage("Send failed", true);
    }
}

void CanConfigWin::onReceiveTimer()
{
    if (!canReady) return;
    can_frame frame;
    while (CanDriver::GetInstance()->receive(frame, 0)) {
        int row = twReceive->rowCount();
        twReceive->insertRow(row);
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        twReceive->setItem(row, 0, new QTableWidgetItem(timestamp));
        twReceive->setItem(row, 1, new QTableWidgetItem(QString("0x%1").arg(frame.can_id, 0, 16)));
        twReceive->setItem(row, 2, new QTableWidgetItem(QString::number(frame.can_dlc)));
        QString dataStr;
        for (int i = 0; i < frame.can_dlc; ++i) {
            dataStr += QString("%1 ").arg(frame.data[i], 2, 16, QChar('0'));
        }
        twReceive->setItem(row, 3, new QTableWidgetItem(dataStr.trimmed()));
        twReceive->setItem(row, 4, new QTableWidgetItem("STD"));
        twReceive->scrollToBottom();
        if (twReceive->rowCount() > 1000) {
            twReceive->removeRow(0);
        }
    }
}

void CanConfigWin::logMessage(const QString &msg, bool isError)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    QString prefix = isError ? "[ERROR] " : "[INFO] ";
    teLog->append(timestamp + " " + prefix + msg);
}

void CanConfigWin::updateCanStatus(bool initialized)
{
    logMessage(initialized ? "CAN ready for communication" : "CAN offline");
}

TPCANHandle CanConfigWin::GetSelectedChannelHandle() const {
    QString device = cbDevice->currentData().toString();
    for (const auto& info : channelList) {
        if (QString::fromStdString(info.channelName) == device) {
            return info.handle;
        }
    }
    return PCAN_NONEBUS;
}