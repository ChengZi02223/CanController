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
    QTextEdit   *teLog;

    // Test 页面组件
    QLineEdit   *leSendId;
    QComboBox   *cbSendDlc;
    QLineEdit   *leSendData;      // 合并后的数据输入框
    QPushButton *btnSend;
    QTableWidget *twReceive;

    QTimer      *receiveTimer;
    bool        canReady;
    std::vector<CanChannelInfo> channelList;
};

#endif //_CAN_CONFIG_WIN_H_