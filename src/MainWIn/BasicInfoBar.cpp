#include "BasicInfoBar.h"

BasicInfoBar::BasicInfoBar(LayoutStyle style, QWidget* parent) 
    : QGroupBox(parent) {
    setTitle("测试基本信息栏");
    InitBar(style);
}

void BasicInfoBar::InitBar(LayoutStyle style) {
    main_layout_ = new QHBoxLayout(this);
    main_layout_->setContentsMargins(5, 5, 5, 5);
    main_layout_->setSpacing(10);

    serial_num_edit_ = new QInfoEdit("序列号：", this);     //序列号
    serial_num_edit_->setAlignment(Qt::AlignCenter);
    // serial_num_edit_->setReadOnly(true);
    serial_num_edit_->setEnabled(false);
    date_edit_ = new QInfoEdit("日期：", this);           //日期
    date_edit_->setAlignment(Qt::AlignCenter);
    // date_edit_->setReadOnly(true);
    date_edit_->setEnabled(false);
    model_edit_ = new QInfoEdit("型号：", this);          //型号  
    model_edit_->setAlignment(Qt::AlignCenter);
    // model_edit_->setReadOnly(true);
    model_edit_->setEnabled(false);
    channel_count_edit_ = new QInfoEdit("联数：", this);  //联数
    channel_count_edit_->setAlignment(Qt::AlignCenter);
    // channel_count_edit_->setReadOnly(true);
    channel_count_edit_->setEnabled(false);
    manufacturer_edit_ = new QInfoEdit("厂家：", this);   //厂家
    manufacturer_edit_->setAlignment(Qt::AlignCenter);
    // manufacturer_edit_->setReadOnly(true);
    manufacturer_edit_->setEnabled(false);
    version_edit_ = new QInfoEdit("版本：", this);        //版本
    version_edit_->setAlignment(Qt::AlignCenter);
    // version_edit_->setReadOnly(true);
    version_edit_->setEnabled(false);
    baud_rate_edit_ = new QInfoEdit("波特率：", this);      //波特率
    baud_rate_edit_->setAlignment(Qt::AlignCenter);
    // baud_rate_edit_->setReadOnly(true);
    baud_rate_edit_->setEnabled(false);
    protocol_edit_ = new QInfoEdit("协议：", this);       //协议
    protocol_edit_->setAlignment(Qt::AlignCenter);
    // protocol_edit_->setReadOnly(true);
    protocol_edit_->setEnabled(false);

    if(style == LayoutStyle::kHar) {
        CreateHarBar();
    } else {
        CrateValBar();
    }

    InitData();
}

void BasicInfoBar::CrateValBar() {
    auto grid_layout = new QGridLayout();

    grid_layout->addWidget(serial_num_edit_, 0, 0);
    grid_layout->addWidget(date_edit_, 0, 1);
    grid_layout->addWidget(model_edit_, 0, 2);
    grid_layout->addWidget(channel_count_edit_, 0, 3);
    grid_layout->addWidget(manufacturer_edit_, 1, 0);
    grid_layout->addWidget(version_edit_, 1, 1);
    grid_layout->addWidget(baud_rate_edit_, 1, 2);
    grid_layout->addWidget(protocol_edit_, 1, 3);
    main_layout_->addLayout(grid_layout);
}

void BasicInfoBar::CreateHarBar() {
    start_btn_ = new QPushButton("测试结束", this);
    start_btn_->setObjectName("StartBtn");
    start_btn_->setFixedHeight(50);
    start_btn_->setMinimumWidth(100);
    main_layout_->addWidget(serial_num_edit_);
    main_layout_->addWidget(date_edit_);
    main_layout_->addWidget(model_edit_);
    main_layout_->addWidget(channel_count_edit_);
    main_layout_->addWidget(manufacturer_edit_);
    main_layout_->addWidget(version_edit_);
    main_layout_->addWidget(baud_rate_edit_);
    main_layout_->addWidget(protocol_edit_);
    main_layout_->addWidget(start_btn_);
    connect(start_btn_, &QPushButton::clicked, this, &BasicInfoBar::OnStartBtnClicked);
}

void BasicInfoBar::SetSerialNum(const QString& serial_num) {
    serial_num_edit_->setInfor(serial_num);
}

QString BasicInfoBar::GetSerialNum() const {
    return serial_num_edit_->text();
}

void BasicInfoBar::SetDate(const QString& date) {
    date_edit_->setInfor(date);
}

QString BasicInfoBar::GetDate() const {
    return date_edit_->text();
}

void BasicInfoBar::SetModel(const QString& model) {
    model_edit_->setInfor(model);
}

QString BasicInfoBar::GetModel() const {
    return model_edit_->text();
}

void BasicInfoBar::SetChannelCount(const QString& channel_count) {
    channel_count_edit_->setInfor(channel_count);
}

QString BasicInfoBar::GetChannelCount() const {
    return channel_count_edit_->text();
}

void BasicInfoBar::SetManufacturer(const QString& manufacturer) {
    manufacturer_edit_->setInfor(manufacturer);
}

QString BasicInfoBar::GetManufacturer() const {
    return manufacturer_edit_->text();
}

void BasicInfoBar::SetVersion(const QString& version) {
    version_edit_->setInfor(version);
}

QString BasicInfoBar::GetVersion() const {
    return version_edit_->text();
}

void BasicInfoBar::SetBaudRate(const QString& baud_rate) {
    baud_rate_edit_->setInfor(baud_rate);
}

QString BasicInfoBar::GetBaudRate() const {
    return baud_rate_edit_->text();
}

void BasicInfoBar::SetProtocol(const QString& protocol) {
    protocol_edit_->setInfor(protocol);
}

QString BasicInfoBar::GetProtocol() const {
    return protocol_edit_->text();
}

void BasicInfoBar::InitData() {
    SetSerialNum("TEST-001");
    SetDate(QDate::currentDate().toString("yyyy-MM-dd"));
    SetModel("液压设备");
    SetChannelCount("2");
    SetManufacturer("0x51");
    SetVersion("V1.2");
    SetBaudRate("2500000");
    SetProtocol("CANopen");
}

void BasicInfoBar::OnStartBtnClicked() {
    if (state_ == kTestStart) {
        start_btn_->setText("测试结束");
        state_ = kTestEnd;
    } else {
        start_btn_->setText("测试开始");
        state_ = kTestStart;
    }
    serial_num_edit_->setDisabled(state_);
    date_edit_->setDisabled(state_);
    model_edit_->setDisabled(state_);
    channel_count_edit_->setDisabled(state_);
    manufacturer_edit_->setDisabled(state_);
    version_edit_->setDisabled(state_);
    baud_rate_edit_->setDisabled(state_);
    protocol_edit_->setDisabled(state_);

    emit SendTestState(state_);
}