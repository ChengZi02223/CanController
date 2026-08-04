#ifndef _CAN_CONFIG_WIN_H_
#define _CAN_CONFIG_WIN_H_

#include "CanDriver.h"
#include <QMainWindow>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QComboBox;
class QPushButton;
class QLineEdit;
class QTextEdit;
class QTableWidget;
class QSpinBox;
class QCheckBox;
QT_END_NAMESPACE

class CanConfigWin : public QMainWindow
{
    Q_OBJECT
public:
    static CanConfigWin* GetInstance(QWidget *parent = nullptr) {
        static CanConfigWin instance(parent);
        return &instance;
    }

private slots:
    void onRefreshDevices();
    void onInit();
    void onRelease();
    void onOpenController();
    void onSend();
    void onReceiveTimer();
    void onChangeMode();

private:
    explicit CanConfigWin(QWidget *parent = nullptr);
    ~CanConfigWin();

    void setupUI();
    void logMessage(const QString &msg, bool isError = false);
    void SendAndReadSDOInfo();
    void InitSDOInfo();
    void updateCanStatus(bool initialized);
    TPCANHandle GetSelectedChannelHandle() const; 

    // UI 组件
    QComboBox   *cbDevice;
    QPushButton *btnRefresh;
    QComboBox   *cbBaudrate;
    QPushButton *btnInit;
    QPushButton *btnRelease;
    QPushButton *btnChange;
    QPushButton *btnController;
    QTextEdit   *teLog;          // 用于显示状态信息

    // Test 页面组件
    QLineEdit   *leSendId;
    QComboBox   *cbSendDlc;
    QLineEdit   *leSendData0;
    QLineEdit   *leSendData1;
    QLineEdit   *leSendData2;
    QLineEdit   *leSendData3;
    QLineEdit   *leSendData4;
    QLineEdit   *leSendData5;
    QLineEdit   *leSendData6;
    QLineEdit   *leSendData7;
    QPushButton *btnSend;
    QTableWidget *twReceive;     // 显示接收的帧

    QTimer      *receiveTimer;
    bool        canReady;
    std::vector<CanChannelInfo> channelList;
};

#endif //_CAN_CONFIG_WIN_H_