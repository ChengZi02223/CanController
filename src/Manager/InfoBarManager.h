#ifndef _INFO_BAR_MANAGER_H_
#define _INFO_BAR_MANAGER_H_

#include <QString>
#include "BasicInfoBar.h"

class InfoBarManager {
public:
    static InfoBarManager* GetInstance();

    BasicInfoBar *GetInfoBar(LayoutStyle style, QWidget* parent = nullptr);
    void UpdateInfoBar();

    void SetSerialNum(const QString& serial_num);
    QString GetSerialNum() const;

    void SetDate(const QString& date);
    QString GetDate() const;

    void SetModel(const QString& model);
    QString GetModel() const;

    void SetChannelCount(const QString& channel_count);
    QString GetChannelCount() const;

    void SetManufacturer(const QString& manufacturer);
    QString GetManufacturer() const;

    void SetVersion(const QString& version);
    QString GetVersion() const;

    void SetBaudRate(const QString& baud_rate);
    QString GetBaudRate() const;

    void SetProtocol(const QString& protocol);
    QString GetProtocol() const;

private:
    InfoBarManager() = default;
    ~InfoBarManager() = default;

    QString serial_num_;
    QString date_;
    QString model_;
    QString channel_count_;
    QString manufacturer_;
    QString version_;
    QString baud_rate_;
    QString protocol_;

    std::vector<BasicInfoBar*> info_bars_;  // 存储所有注册的BasicInfoBar实例
};















#endif // _INFO_BAR_MANAGER_H_