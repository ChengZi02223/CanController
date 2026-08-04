#ifndef _CMD_MANAGER_H_
#define _CMD_MANAGER_H_

#include <QObject>
#include <QMutex>
#include <QMap>
#include <functional>
#include <cstdint>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>          // 新增
#include <QDateTime>       // 用于时间戳

#include "CanDriver.h"
#include "CO_CMD.h"

// 节点信息结构
struct NodeInfo {
    uint8_t nodeId;
    uint8_t nmtState;
    uint64_t lastHeartbeatTime;      // 毫秒时间戳
    uint32_t heartbeatTimeout;
    bool heartbeatEnabled;
    QMap<uint32_t, std::function<void(const std::vector<uint8_t>&)>> tpdCallbacks;
};

struct NodeSDOInfo {
    uint32_t deviceType = 0;
    uint8_t  protocol   = 0;
    uint32_t vendorId   = 0;
    uint16_t status     = 0;
    uint16_t fault      = 0;
};

class CmdManager : public QObject {
    Q_OBJECT
public:
    static CmdManager* GetInstance() {
        static CmdManager* instance = nullptr;
        if (instance == nullptr) {
            instance = new CmdManager();
        }
        return instance;
    }

    // 节点管理
    bool addNode(uint8_t nodeId, uint32_t heartbeatTimeoutMs = 500);
    bool removeNode(uint8_t nodeId);
    void setHeartbeatTimeout(uint8_t nodeId, uint32_t timeoutMs);

    // 启动/停止
    void start();
    void stop();
    bool isRunning() const { return m_running; }

    // NMT命令
    void sendNmt(uint8_t nodeId, uint8_t cmd);

    // SDO操作（异步回调）
    using SdoCallback = std::function<void(bool success, const std::vector<uint8_t>& data)>;
    void sdoRead(uint8_t nodeId, uint16_t index, uint8_t subIndex, SdoCallback callback);
    void sdoWrite(uint8_t nodeId, uint16_t index, uint8_t subIndex, const std::vector<uint8_t>& data, SdoCallback callback);

    // PDO
    bool sendRPDO(uint8_t nodeId, uint8_t pdoNum, const std::vector<uint8_t>& data);
    void registerTpdoCallback(uint32_t cobId, std::function<void(const std::vector<uint8_t>&)> callback);

    void setSdoTimeout(uint32_t ms) { m_sdoTimeoutMs = ms; }

signals:
    void heartbeatTimeout(uint8_t nodeId);
    void nmtStateChanged(uint8_t nodeId, uint8_t newState);
    void tpdoReceived(uint32_t cobId, const std::vector<uint8_t>& data);
    void emergencyReceived(uint8_t nodeId, uint16_t errorCode, const std::vector<uint8_t>& additional);

private:
    explicit CmdManager(QObject *parent = nullptr);
    ~CmdManager();

    // 线程入口函数
    void receiverLoop();
    void heartbeatLoop();

    // 内部处理函数
    void processFrame(uint32_t cobId, const std::vector<uint8_t>& data);
    void processHeartbeat(uint8_t nodeId, uint8_t state);
    void processSdoResponse(uint8_t nodeId, const std::vector<uint8_t>& data);
    void processEmergency(uint8_t nodeId, const std::vector<uint8_t>& data);
    void processTpdo(uint32_t cobId, const std::vector<uint8_t>& data);
    void checkSdoTimeouts();

    // 底层CAN封装（线程安全，内部加锁）
    bool sendFrame(uint32_t id, const std::vector<uint8_t>& data);
    bool receiveFrame(uint32_t& id, std::vector<uint8_t>& data, int timeoutMs);

    // 工具
    uint32_t makeTransactionKey(uint8_t nodeId, uint16_t index) const {
        return (static_cast<uint32_t>(nodeId) << 16) | index;
    }

    // 线程控制
    std::atomic<bool> m_running;
    std::thread m_receiverThread;
    std::thread m_heartbeatThread;

    // 节点数据
    mutable QMutex m_nodeMutex;
    QMap<uint8_t, NodeInfo> m_nodes;

    // SDO事务
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
    QMap<uint32_t, SdoTransaction> m_sdoTransactions;
    uint32_t m_sdoTimeoutMs;

    // （可选）保护CAN发送的互斥锁，如果CanDriver本身线程安全可不加
    QMutex m_canMutex;
};

#endif // _CMD_MANAGER_H_