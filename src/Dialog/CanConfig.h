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

enum ControlMode {
    kMode_PCAN,
    kMode_J1939
};

class CanConfigWin : public QMainWindow
{
    Q_OBJECT
public:
    static CanConfigWin* GetInstance(QWidget *parent = nullptr) {
        static CanConfigWin instance(parent);
        return &instance;
    }

signals:
    void SendReadFromEPROM(can_frame frame);

private slots:
    void onRefreshDevices();
    void onInit();
    void onRelease();
    void onOpenController();
    void onSend();
    void onAddCmd();
    void onDeleteCmd();
    void onReceiveTimer();
    void onChangeMode();

    void onExportTxt();

private:
    explicit CanConfigWin(QWidget *parent = nullptr);
    ~CanConfigWin();

    void setupUI();
    void logMessage(const QString &msg, bool isError = false);
    void updateCanStatus(bool initialized);
    TPCANHandle GetSelectedChannelHandle() const; 
    void SendData(uint32_t id, uint8_t dlc, QString data);

    ControlMode control_mode_ = kMode_J1939;  // 默认使用J1939模式
    bool on_set_one_ = true;

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
    QTableWidget *moreCmdTable_ = nullptr;

    QTimer      *receiveTimer;
    bool        canReady;
    std::vector<CanChannelInfo> channelList;
    bool on_test_ = false;
};

#endif //_CAN_CONFIG_WIN_H_