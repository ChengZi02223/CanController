#ifndef _CMD_MANAGER_H_
#define _CMD_MANAGER_H_

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QMap>
#include <QTimer>
#include <functional>
#include <cstdint>
#include <vector>
#include <memory>

#include "CanDriver.h"
#include "CO_CMD.h"   // CANopen命令宏定义

// 节点信息结构
struct NodeInfo {
    uint8_t nodeId;
    uint8_t nmtState;                // 使用 HEART_STATE_* 宏
    uint64_t lastHeartbeatTime;      // 毫秒时间戳
    uint32_t heartbeatTimeout;       // 超时阈值（ms）
    bool heartbeatEnabled;
    QMap<uint32_t, std::function<void(const std::vector<uint8_t>&)>> tpdCallbacks;
};

// 管理类
class CmdManager : public QObject {
    Q_OBJECT
public:
    explicit CmdManager(const std::string& deviceName, uint32_t baudrate, QObject *parent = nullptr);
    ~CmdManager();

    // 节点管理
    bool addNode(uint8_t nodeId, uint32_t heartbeatTimeoutMs = 500);
    bool removeNode(uint8_t nodeId);
    void setHeartbeatTimeout(uint8_t nodeId, uint32_t timeoutMs);

    // 启动/停止
    void start();
    void stop();
    bool isRunning() const { return m_running; }

    // NMT命令（节点ID=0广播）
    void sendNmt(uint8_t nodeId, uint8_t cmd);  // cmd 使用 NMT_CMD_* 宏

    // SDO操作（异步，通过回调返回结果）
    using SdoCallback = std::function<void(bool success, const std::vector<uint8_t>& data)>;
    void sdoRead(uint8_t nodeId, uint16_t index, uint8_t subIndex, SdoCallback callback);
    void sdoWrite(uint8_t nodeId, uint16_t index, uint8_t subIndex, const std::vector<uint8_t>& data, SdoCallback callback);

    // PDO发送（RPDO）
    bool sendRPDO(uint8_t nodeId, uint8_t pdoNum, const std::vector<uint8_t>& data); // pdoNum: 1~3

    // 注册TPDO接收回调
    void registerTpdoCallback(uint32_t cobId, std::function<void(const std::vector<uint8_t>&)> callback);

    // 设置SDO超时（默认1000ms）
    void setSdoTimeout(uint32_t ms) { m_sdoTimeoutMs = ms; }

signals:
    void heartbeatTimeout(uint8_t nodeId);
    void nmtStateChanged(uint8_t nodeId, uint8_t newState);  // newState 使用 HEART_STATE_* 宏
    void tpdoReceived(uint32_t cobId, const std::vector<uint8_t>& data);
    void emergencyReceived(uint8_t nodeId, uint16_t errorCode, const std::vector<uint8_t>& additional);

private slots:
    void onHeartbeatTimer();

private:
    CanDriver m_canDriver;
    QThread m_receiverThread;
    QTimer m_heartbeatTimer;
    volatile bool m_running;

    mutable QMutex m_nodeMutex;
    QMap<uint8_t, NodeInfo> m_nodes;

    // SDO事务管理
    struct SdoTransaction {
        uint8_t nodeId;
        uint16_t index;
        uint8_t subIndex;
        SdoCallback callback;
        uint64_t startTime;
        bool isWrite;
        uint8_t toggle;
        std::vector<uint8_t> accumulatedData;
    };
    mutable QMutex m_sdoMutex;
    QMap<uint32_t, SdoTransaction> m_sdoTransactions; // key = (nodeId << 16) | index
    uint32_t m_sdoTimeoutMs;

    void receiverLoop();
    void processFrame(uint32_t cobId, const std::vector<uint8_t>& data);
    void processHeartbeat(uint8_t nodeId, uint8_t state);
    void processSdoResponse(uint8_t nodeId, const std::vector<uint8_t>& data);
    void processEmergency(uint8_t nodeId, const std::vector<uint8_t>& data);
    void processTpdo(uint32_t cobId, const std::vector<uint8_t>& data);

    void sendSdoRequest(uint8_t nodeId, uint16_t index, uint8_t subIndex, const std::vector<uint8_t>& data, bool isWrite);

    uint32_t makeTransactionKey(uint8_t nodeId, uint16_t index) const {
        return (static_cast<uint32_t>(nodeId) << 16) | index;
    }
    void checkSdoTimeouts();

    // 底层CAN封装
    bool sendFrame(uint32_t id, const std::vector<uint8_t>& data);
    bool receiveFrame(uint32_t& id, std::vector<uint8_t>& data, int timeoutMs);
};

#endif // _CMD_MANAGER_H_