#include "InfoBarManager.h"

InfoBarManager* InfoBarManager::GetInstance() {
    static InfoBarManager * instance = nullptr;
    if(!instance)
        instance = new InfoBarManager();
    return instance;
}

BasicInfoBar *InfoBarManager::GetInfoBar(LayoutStyle style, QWidget* parent) {
    // 如果不存在，则创建新的BasicInfoBar实例
    BasicInfoBar* newBar = new BasicInfoBar(style, parent);
    info_bars_.push_back(newBar);
    return newBar;
}

void InfoBarManager::UpdateInfoBar() {
    for (BasicInfoBar* bar : info_bars_) {
        if (bar) {
            bar->SetSerialNum(serial_num_);
            bar->SetDate(date_);
            bar->SetModel(model_);
            bar->SetChannelCount(channel_count_);
            bar->SetManufacturer(manufacturer_);
            bar->SetVersion(version_);
            bar->SetBaudRate(baud_rate_);
            bar->SetProtocol(protocol_);
        }
    }
}

void InfoBarManager::SetSerialNum(const QString& serial_num) {
    serial_num_ = serial_num;
    UpdateInfoBar();
}

QString InfoBarManager::GetSerialNum() const {
    return serial_num_;
}

void InfoBarManager::SetDate(const QString& date) {
    date_ = date;
    UpdateInfoBar();
}

QString InfoBarManager::GetDate() const {
    return date_;
}

void InfoBarManager::SetModel(const QString& model) {
    model_ = model;
    UpdateInfoBar();
}

QString InfoBarManager::GetModel() const {
    return model_;
}

void InfoBarManager::SetChannelCount(const QString& channel_count) {
    channel_count_ = channel_count;
    UpdateInfoBar();
}

QString InfoBarManager::GetChannelCount() const {
    return channel_count_;
}

void InfoBarManager::SetManufacturer(const QString& manufacturer) {
    manufacturer_ = manufacturer;
    UpdateInfoBar();
}

QString InfoBarManager::GetManufacturer() const {
    return manufacturer_;
}

void InfoBarManager::SetVersion(const QString& version) {
    version_ = version;
    UpdateInfoBar();
}

QString InfoBarManager::GetVersion() const {
    return version_;
}

void InfoBarManager::SetBaudRate(const QString& baud_rate) {
    baud_rate_ = baud_rate;
    UpdateInfoBar();
}

QString InfoBarManager::GetBaudRate() const {
    return baud_rate_;
}

void InfoBarManager::SetProtocol(const QString& protocol) {
    protocol_ = protocol;
    UpdateInfoBar();
}

QString InfoBarManager::GetProtocol() const {
    return protocol_;
}