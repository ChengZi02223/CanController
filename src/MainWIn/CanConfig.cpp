#include "CanConfig.h"
#include "QtWidgets.h"
#include "CanController.h"
#include "CanDriver.h"

#include <iomanip>
#include <sstream>
#include <unordered_map>

static std::string FormatChannelName(TPCANHandle handle, DWORD features) {
    std::string name;
    auto it = DevToStrMap.find(handle);
    if (it != DevToStrMap.end()) {
        name = it->second;
    }
    if (features & FEATURE_FD_CAPABLE) name += " (FD)";
    if (features & FEATURE_XL_CAPABLE) name += " (XL)";
    return name;
}

CanConfigWin::CanConfigWin(QWidget *parent)
    : QMainWindow(parent), canReady(false)
{
    setupUI();
    receiveTimer = new QTimer(this);
    connect(receiveTimer, &QTimer::timeout, this, &CanConfigWin::onReceiveTimer);
    receiveTimer->start(50);  // 50ms 轮询接收
    onRefreshDevices();       // 初始刷新设备列表
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
    cbBaudrate->setCurrentIndex(2); // 500k

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
    btnController = new QPushButton("Controller");
    btnController->setObjectName("ControllerBtn");
    btnController->setFixedSize(100, 30);
    //TODO: set for false normally
    btnController->setEnabled(true);
    btnRelease->setEnabled(false);
    btnLayout->addWidget(btnInit);
    btnLayout->addWidget(btnRelease);
    btnLayout->addStretch();
    btnLayout->addWidget(btnController, 0, Qt::AlignRight);

    teLog = new QTextEdit;
    teLog->setReadOnly(true);
    // teLog->setMaximumHeight(150);

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

    QGridLayout *dataLayout = new QGridLayout;
    dataLayout->setSpacing(0);
    for (int i = 0; i < 8; ++i) {
        QLineEdit *le = new QLineEdit("00");
        le->setFixedWidth(60);
        dataLayout->addWidget(new QLabel(QString("Byte %1:").arg(i)), 0, i);
        dataLayout->addWidget(le, 1, i);
        // 保存指针以便发送时获取
        switch (i) {
            case 0: leSendData0 = le; break;
            case 1: leSendData1 = le; break;
            case 2: leSendData2 = le; break;
            case 3: leSendData3 = le; break;
            case 4: leSendData4 = le; break;
            case 5: leSendData5 = le; break;
            case 6: leSendData6 = le; break;
            case 7: leSendData7 = le; break;
        }
    }
    sendLayout->addLayout(dataLayout);
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
    connect(btnController, &QPushButton::clicked, this, &CanConfigWin::onOpenController);
    connect(btnSend, &QPushButton::clicked, this, &CanConfigWin::onSend);
}

void CanConfigWin::onRefreshDevices()
{
    cbDevice->clear();
    uint32_t channelCount = 0;
    TPCANStatus st = CAN_GetValue(PCAN_NONEBUS, PCAN_ATTACHED_CHANNELS_COUNT, &channelCount, sizeof(channelCount));
    if (st != PCAN_ERROR_OK || channelCount == 0) {
        btnInit->setEnabled(false);
        return;
    }

    // 分配缓冲区获取通道信息
    TPCANChannelInformation* info = new TPCANChannelInformation[channelCount];
    st = CAN_GetValue(PCAN_NONEBUS, PCAN_ATTACHED_CHANNELS, info, channelCount * sizeof(TPCANChannelInformation));
    if (st == PCAN_ERROR_OK) {
        btnInit->setEnabled(true);
        for (uint32_t i = 0; i < channelCount; ++i) {
            if (info[i].channel_condition & PCAN_CHANNEL_AVAILABLE) {
                std::string name = FormatChannelName(info[i].channel_handle, info[i].device_features);
                cbDevice->addItem(QString::fromStdString(name), QString::fromStdString(name));
            }
        }
    }
    delete[] info;
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
    unsigned int baudrate = 125000;
    switch (baudIdx) {
        case 0: baudrate = 125000; break;
        case 1: baudrate = 250000; break;
        case 2: baudrate = 500000; break;
        case 3: baudrate = 1000000; break;
    }
    if (canDriver.init(device.toStdString(), baudrate)) {
        canReady = true;
        logMessage(QString("Init OK: %1 @ %2 bps").arg(device).arg(baudrate));
        btnInit->setEnabled(false);
        btnRelease->setEnabled(true);
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
    canDriver.close();
    canReady = false;
    logMessage("CAN released.");
    btnInit->setEnabled(true);
    btnRelease->setEnabled(false);
    btnController->setEnabled(false);
    btnSend->setEnabled(false);
    updateCanStatus(false);
}

void CanConfigWin::onOpenController() {
    this->close();
    CanController *w  = new CanController();
    w->show();
}

void CanConfigWin::onSend()
{
    if (!canReady) {
        logMessage("CAN not initialized, cannot send.", true);
        return;
    }
    can_frame frame;
    bool ok;
    frame.can_id = leSendId->text().toUInt(&ok, 16);
    if (!ok) {
        logMessage("Invalid ID (hex)", true);
        return;
    }
    frame.can_dlc = cbSendDlc->currentText().toUInt();
    // 收集数据字节
    QLineEdit* dataLe[8] = {leSendData0, leSendData1, leSendData2, leSendData3,
                            leSendData4, leSendData5, leSendData6, leSendData7};
    for (int i = 0; i < frame.can_dlc; ++i) {
        frame.data[i] = static_cast<uint8_t>(dataLe[i]->text().toUInt(&ok, 16));
        if (!ok) {
            logMessage(QString("Invalid data byte %1").arg(i), true);
            return;
        }
    }
    if (canDriver.send(frame)) {
        logMessage(QString("Sent: ID=0x%1 DLC=%2").arg(frame.can_id, 0, 16).arg(frame.can_dlc));
    } else {
        logMessage("Send failed", true);
    }
}

void CanConfigWin::onReceiveTimer()
{
    if (!canReady) return;
    can_frame frame;
    // 非阻塞读取（超时0）
    while (canDriver.receive(frame, 0)) {
        // 添加一行到表格
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
        // 限制最大行数
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
    // 可改变界面元素颜色等，简单记录日志
    logMessage(initialized ? "CAN ready for communication" : "CAN offline");
}