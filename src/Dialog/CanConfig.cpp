#include "CanConfig.h"
#include "QtWidgets.h"
#include "CanController.h"
#include "CO_CMD.h"
#include "Utils.h"
#include "can_cmd.h"

#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <QScrollBar>

CanConfigWin::CanConfigWin(QWidget *parent)
    : QMainWindow(parent), canReady(false)
{
    setupUI();
    receiveTimer = new QTimer(this);
    connect(receiveTimer, &QTimer::timeout, this, &CanConfigWin::onReceiveTimer);
    setWindowIcon(QIcon(":/icons/HZLK.png"));
#ifndef ON_TEST_MODE
    receiveTimer->start(50);
#endif
}

CanConfigWin::~CanConfigWin()
{
    if (canReady) onRelease();
}

void CanConfigWin::setupUI()
{
    setWindowTitle("CAN Controller - PCAN Style");
    resize(1000, 800);

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
    btnChange = new QPushButton("J1939");
    btnChange->setObjectName("ChangeBtn");
    btnChange->setFixedSize(100, 30);
    btnChange->setToolTip("Current mode: J1939. Click to switch to CANOpen.");
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

    QGroupBox *sendOneCmdGroup = new QGroupBox("Send One");
    sendOneCmdGroup->setCheckable(true);
    sendOneCmdGroup->setChecked(true);
    QVBoxLayout *sendOneLayout = new QVBoxLayout(sendOneCmdGroup);

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
    sendOneLayout->addLayout(idLayout);

    // 合并后的数据输入框
    QHBoxLayout *dataLayout = new QHBoxLayout;
    dataLayout->addWidget(new QLabel("Data (Hex):"));
    leSendData = new QLineEdit;
    leSendData->setPlaceholderText("Enter hex bytes, e.g., 01 02 03 04 05 06 07 08");
    leSendData->setFixedHeight(30);
    dataLayout->addWidget(leSendData, 1);  // 1表示拉伸
    sendOneLayout->addLayout(dataLayout);

    // 添加提示标签
    QLabel *hintLabel = new QLabel("Tip: Enter hexadecimal bytes separated by spaces");
    hintLabel->setStyleSheet("color: gray; font-size: 9pt;");
    sendOneLayout->addWidget(hintLabel);

    QGroupBox *sendMoreCmdGroup = new QGroupBox("Send More");
    sendMoreCmdGroup->setCheckable(true);
    sendMoreCmdGroup->setChecked(false);
    QHBoxLayout *sendMoreLayout = new QHBoxLayout(sendMoreCmdGroup);

    moreCmdTable_ = new QTableWidget(sendMoreCmdGroup);
    moreCmdTable_->setColumnCount(3);
    moreCmdTable_->setHorizontalHeaderLabels({"ID", "DLC", "Data"});
    moreCmdTable_->verticalHeader()->setVisible(false);
    moreCmdTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    moreCmdTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    moreCmdTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    moreCmdTable_->setColumnWidth(0, 150);
    moreCmdTable_->setColumnWidth(1, 50);
    moreCmdTable_->setMinimumSize(270, 100);
    moreCmdTable_->setSelectionBehavior(QAbstractItemView::SelectRows);

    QVBoxLayout *more_btn_layout = new QVBoxLayout();
    QPushButton *add_cmd_btn = new QPushButton("Add");
    add_cmd_btn->setMinimumWidth(30);
    QPushButton *delete_cmd_btn = new QPushButton("Delete");
    delete_cmd_btn->setMinimumWidth(30);
    more_btn_layout->addWidget(add_cmd_btn);
    more_btn_layout->addWidget(delete_cmd_btn);

    sendMoreLayout->addWidget(moreCmdTable_);
    sendMoreLayout->addLayout(more_btn_layout);

    sendLayout->addWidget(sendOneCmdGroup);
    sendLayout->addWidget(sendMoreCmdGroup);
    btnSend = new QPushButton("Send");
    btnSend->setFixedSize(100, 30);
    btnSend->setObjectName("SendBtn");
    btnSend->setFixedWidth(150);
#ifdef ON_TEST_MODE
    btnSend->setEnabled(true);
#else
    btnSend->setEnabled(false);
#endif
    sendLayout->addWidget(btnSend, 1, Qt::AlignRight);
    sendLayout->addStretch();

    // 接收区
    QGroupBox *recvGroup = new QGroupBox("Received Messages");
    QVBoxLayout *recvLayout = new QVBoxLayout(recvGroup);
    twReceive = new QTableWidget(0, 5);
    twReceive->setHorizontalHeaderLabels({"Time", "ID (Hex)", "DLC", "Data", "Type"});
    twReceive->horizontalHeader()->setStretchLastSection(true);
    recvLayout->addWidget(twReceive);

    auto stop_btn = new QPushButton("Stop");
    stop_btn->setFixedSize(130, 30);
    stop_btn->setCheckable(true);
    auto save_btn = new QPushButton("Save");
    save_btn->setFixedSize(130, 30);
    save_btn->setEnabled(false);

    QHBoxLayout *btn_layout = new QHBoxLayout();
    btn_layout->addStretch();
    btn_layout->addWidget(stop_btn, 0, Qt::AlignRight);
    btn_layout->addWidget(save_btn, 0, Qt::AlignRight);
    recvLayout->addLayout(btn_layout);

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
    connect(add_cmd_btn, &QPushButton::clicked, this, &CanConfigWin::onAddCmd);
    connect(delete_cmd_btn, &QPushButton::clicked, this, &CanConfigWin::onDeleteCmd);
    connect(save_btn, &QPushButton::clicked, this, &CanConfigWin::onExportTxt);
    connect(stop_btn, &QPushButton::clicked, this, [this, save_btn](bool checked) {
        on_test_ = checked;
        save_btn->setEnabled(checked);
#ifdef ON_TEST_MODE
        if(!on_test_) {
            receiveTimer->start(50);
        } else {
            receiveTimer->stop();
        }
#endif
    });

    connect(sendOneCmdGroup, &QGroupBox::toggled, this, [=](bool checked){
        if(checked){
            sendMoreCmdGroup->setChecked(false);
        }else{
            // 禁止取消，如果另一个也没选，强制勾回来
            if(!sendMoreCmdGroup->isChecked()){
                sendOneCmdGroup->setChecked(true);
            }
        }
        on_set_one_ = true;
    });

    connect(sendMoreCmdGroup, &QGroupBox::toggled, this, [=](bool checked){
        if(checked){
            sendOneCmdGroup->setChecked(false);
        }else{
            if(!sendOneCmdGroup->isChecked()){
                sendMoreCmdGroup->setChecked(true);
            }
        }
        on_set_one_ = false;
    });

}

void CanConfigWin::onRefreshDevices()
{
    cbDevice->clear();
    channelList.clear();

    channelList = CanDriver::GetInstance()->scanAllChannels();
    if (channelList.empty()) {
        QMessageBox::information(this, "提示", "未检测到PCAN设备, 请检查USB/驱动");
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
    can_frame frame;
    bool res = false;
    if(control_mode_ == kMode_J1939) {
        // 切换成CANOpen模式
        res = CanDriver::GetInstance()->ExecCmd(CHANGE_TO_CANOPEN_COB_ID, CHANGE_TO_CANOPEN_CMD, frame, 200);
    } else {
        // 切换成J1939模式
        res = CanDriver::GetInstance()->ExecCmd(SDO_COB_ID, CHANGE_TO_J1939_CMD, frame, 200);
    }
    if(!res) {
        logMessage("Mode change command failed!", true);
        return;
    }
    auto responseId = frame.can_id;
    auto response = frame.data;
    if(control_mode_ == kMode_J1939) {
        control_mode_ = kMode_PCAN;
        btnChange->setText("CANOpen");
        btnChange->setToolTip("Current mode: CANOpen. Click to switch to J1939.");
    } else if (control_mode_ == kMode_PCAN) {
        control_mode_ = kMode_J1939;
        btnChange->setText("J1939");
        btnChange->setToolTip("Current mode: J1939. Click to switch to CANOpen.");
    }
}

void CanConfigWin::onOpenController() {
    this->close();
    CanController::GetInstance()->show();
}

void CanConfigWin::onAddCmd() {
    int row = moreCmdTable_->rowCount(); // 在末尾新增行号
    moreCmdTable_->insertRow(row);
    moreCmdTable_->setItem(row, 0, new QTableWidgetItem(""));
    moreCmdTable_->setItem(row, 1, new QTableWidgetItem(""));
    moreCmdTable_->setItem(row, 2, new QTableWidgetItem(""));
}

void CanConfigWin::onDeleteCmd(){
    int curRow = moreCmdTable_->currentRow();
    if(curRow < 0) {
        return;
    }
    moreCmdTable_->removeRow(curRow);
}

void CanConfigWin::onSend()
{
    if (!canReady) {
        logMessage("CAN not initialized, cannot send.", true);
        return;
    }
    
    if (on_set_one_) {
        bool ok = false;
        auto id = leSendId->text().toUInt(&ok, 16);
        if (!ok) {
            logMessage("Invalid ID (hex)", true);
            return;
        }
        SendData(id, cbSendDlc->currentText().toUInt(), leSendData->text().trimmed());
    } else {
        if(moreCmdTable_->rowCount() == 0) {
            logMessage("Message table is Empty", true);
            return;
        }
        for(int i = 0; i < moreCmdTable_->rowCount(); ++i) {
            bool ok = false;
            auto id = moreCmdTable_->item(i, 0)->text().toUInt(&ok, 16);
            if (!ok) {
                logMessage("Invalid ID (hex)", true);
                return;
            }
            auto dlc = moreCmdTable_->item(i, 1)->text().toUInt();
            auto data = moreCmdTable_->item(i, 2)->text().trimmed();
            SendData(id, dlc, data);
        }
    }

}

void CanConfigWin::SendData(uint32_t id, uint8_t dlc, QString data) {
    can_frame frame;
    
    // 解析ID
    frame.can_id = id;
    if(frame.can_id > 0x7FF){
        frame.can_id |= CAN_EFF_FLAG;
    }

    // 获取DLC
    frame.can_dlc = dlc;
    if (frame.can_dlc > 8) {
        logMessage("DLC cannot exceed 8", true);
        return;
    }

    // 解析数据 - 从合并的输入框中读取
    if (data.isEmpty()) {
        logMessage("Data input is empty!", true);
        return;
    }

    // 按空格分隔并解析十六进制
    QStringList hexList = data.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    
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
#ifdef ON_TEST_MODE
    can_frame frame = {
        0x123,          // can_id
        8,              // can_dlc，实际使用4个字节
        {0x11,0x22,0x33,0x44,0x09,0x92,0x00,0x00}  // data[8]，必须写满8个或者用{}
    };
#else
    if (!canReady) return;
    can_frame frame;
#endif
    QScrollBar* vBar = twReceive->verticalScrollBar();
    bool needAutoScroll = (vBar->value() >= vBar->maximum() - 2);
#ifndef ON_TEST_MODE
    while (CanDriver::GetInstance()->receive(frame, 0)) {
#endif
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
#ifndef ON_TEST_MODE
    }
#endif
    if(needAutoScroll) twReceive->scrollToBottom();
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

void CanConfigWin::onExportTxt()
{
    // 弹出保存文件对话框，选择保存位置
    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("保存CAN接收记录"),
        QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + "_can_log.txt",
        tr("Text Files (*.txt);;All Files (*)")
    );

    if(filePath.isEmpty()){
        // 用户点取消，直接返回
        return;
    }

    QFile file(filePath);
    // 文本模式，换行使用本地格式
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, tr("错误"), tr("文件打开失败：%1").arg(file.errorString()));
        return;
    }

    QTextStream out(&file);
    // 写入表头，用\t制表符分隔，方便后续Excel直接打开
    out << "Time\tID(Hex)\tDLC\tData\tType\n";

    int rowCount = twReceive->rowCount();
    //遍历表格全部行
    for(int row = 0; row < rowCount; row++)
    {
        QString timeVal  = twReceive->item(row,0) ? twReceive->item(row,0)->text() : "";
        QString idVal    = twReceive->item(row,1) ? twReceive->item(row,1)->text() : "";
        QString dlcVal   = twReceive->item(row,2) ? twReceive->item(row,2)->text() : "";
        QString dataVal  = twReceive->item(row,3) ? twReceive->item(row,3)->text() : "";
        QString typeVal  = twReceive->item(row,4) ? twReceive->item(row,4)->text() : "";

        out << timeVal << "\t"
            << idVal   << "\t"
            << dlcVal  << "\t"
            << dataVal << "\t"
            << typeVal << "\n";
    }

    file.close();
    QMessageBox::information(this, tr("完成"), tr("共导出 %1 条记录").arg(rowCount));
}
