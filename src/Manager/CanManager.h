#ifndef _CAN_MANAGER_H_
#define _CAN_MANAGER_H_

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

enum BatchReadState
{
    kBatchRead_Idle,        // 空闲
    kBatchRead_WaitAck,     // 已经发送0x640，等待0x5C0应答
    kBatchRead_Receiving    // 收到0x5C0，正在接收0x4C0批量数据帧
};

struct NodeInfo {
    uint8_t nodeId;
    uint8_t nmtState;
    uint64_t lastHeartbeatTime;      // 毫秒时间戳
    uint32_t heartbeatTimeout;
    bool heartbeatEnabled;
    QMap<uint32_t, std::function<void(const std::vector<uint8_t>&)>> tpdCallbacks;
};

class CanManager : public QObject {
    Q_OBJECT
public:
    static CanManager* GetInstance() {
        static CanManager* instance = nullptr;
        if (instance == nullptr) {
            instance = new CanManager();
        }
        return instance;
    }

    // 节点管理
    bool AddNode(uint8_t nodeId, uint32_t heartbeatTimeoutMs = 500);
    bool RemoveNode(uint8_t nodeId);
    void SetHeartbeatTimeout(uint8_t nodeId, uint32_t timeoutMs);

    // 启动/停止
    void Start();
    void Stop();
    bool IsRunning() const { return m_running; }

    bool SendFrame(uint32_t id, const std::vector<uint8_t>& data);


signals:
    void SendRowValue(QString value, QString idx, QString sub_idx);
private:
    explicit CanManager(QObject *parent = nullptr);
    ~CanManager();

    // 线程入口函数
    void ReceiverLoop();

    // 底层CAN封装（线程安全，内部加锁）
    
    bool ReceiveFrame(uint32_t& id, std::vector<uint8_t>& data, int timeoutMs);

    void ProcessFrame(uint32_t cobId, const std::vector<uint8_t>& data);
    void Parse0x4C0Frame(const std::vector<uint8_t>& data);

    // 线程控制
    std::atomic<bool> m_running;
    std::thread m_receiverThread;

    // 节点数据
    mutable QMutex m_nodeMutex;
    QMap<uint8_t, NodeInfo> m_nodes;

    QMutex m_canMutex;
    BatchReadState m_batchReadState = kBatchRead_Idle;
};

#endif // _CAN_MANAGER_H_