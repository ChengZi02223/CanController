#ifndef _BASIC_INFO_BAR_H_
#define _BASIC_INFO_BAR_H_

#include "QtWidgets.h"

enum LayoutStyle {
    kHar,
    kVal
};

enum TestState {
    kTestStart,
    kTestEnd
};

class BasicInfoBar : public QGroupBox {
    Q_OBJECT
public:
    BasicInfoBar(LayoutStyle style, QWidget* parent = nullptr);
    ~BasicInfoBar(){}
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

signals:
    void SendTestState(TestState state);

private slots:
    void OnStartBtnClicked();

private:
    void InitBar(LayoutStyle style);
    void CreateHarBar();
    void CrateValBar();
    void InitData();

private:
    QHBoxLayout* main_layout_ = nullptr;
    QInfoEdit* serial_num_edit_ = nullptr;     //序列号
    QInfoEdit* date_edit_ = nullptr;           //日期
    QInfoEdit* model_edit_ = nullptr;          //型号
    QInfoEdit* channel_count_edit_ = nullptr;  //联数
    QInfoEdit* manufacturer_edit_ = nullptr;   //厂家
    QInfoEdit* version_edit_ = nullptr;        //版本
    QInfoEdit* baud_rate_edit_ = nullptr;      //波特率
    QInfoEdit* protocol_edit_ = nullptr;       //协议
    QPushButton* start_btn_ = nullptr;         //测试开始按钮

    TestState state_ = kTestEnd;
};









#endif // _BASIC_INFO_H_