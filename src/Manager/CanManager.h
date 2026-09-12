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
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <deque>
#include <QDateTime>

#include "CanDriver.h"
#include "CO_CMD.h"

enum BatchReadState
{
    kBatchRead_Idle,
    kBatchRead_WaitAck,
    kBatchRead_Receiving
};

struct NodeInfo {
    uint8_t nodeId;
    uint8_t nmtState;
    uint64_t lastHeartbeatTime;
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

    bool AddNode(uint8_t nodeId, uint32_t heartbeatTimeoutMs = 500);
    bool RemoveNode(uint8_t nodeId);
    void SetHeartbeatTimeout(uint8_t nodeId, uint32_t timeoutMs);

    void Start();
    void Stop();
    bool IsRunning() const { return m_running; }

    void PushCommand(uint32_t cobId, const std::vector<uint8_t>& cmd);
    void PushCommand(const CanCmdItem& item);
    void PushCommands(const std::vector<CanCmdItem>& items);

    void ClearCommands();
    size_t CommandCount();

    // 接收缓冲区
    void   TakeReceiveBuffer(std::vector<can_frame>& out, size_t maxBatch = 1024);
    size_t ReceiveBufferSize() const;
    void   ClearReceiveBuffer();
    void   SetReceiveBufferLimit(size_t limit);

    // ========== 【新增】TPDO 信号节流 ==========
    // 0 表示不节流（每帧都发），>0 表示两次 emit 的最小间隔（毫秒）
    // 默认 20ms（≈50Hz），足以让曲线绘制流畅，又不会拖死 GUI 线程
    void SetTpdoEmitIntervalMs(int ms) { m_tpdoEmitIntervalMs_.store(ms); }
    int  TpdoEmitIntervalMs() const    { return m_tpdoEmitIntervalMs_.load(); }

signals:
    void SendRowValue(QString value, QString idx, QString sub_idx);

    void SendStartLoadToTable(can_frame frame);
    void SendSaveToEEPROMDone();
    void SendSaveToDefaultDone();

    void SendReadFromEPROM(can_frame frame);

    void SendTpdo2Position(can_frame frame);
    void SendTpdo3Current(can_frame frame);

public slots:
    void OnStopNMTRead();

private:
    explicit CanManager(QObject *parent = nullptr);
    ~CanManager();

    void ReceiverLoop();
    void SenderLoop();

    bool ReceiveFrame(can_frame &frame);
    void ProcessFrame(const can_frame &frame);

    void EnqueueReceiveFrame(const can_frame& frame);

private:
    std::atomic<bool> m_running;
    std::thread m_receiverThread;
    std::thread m_senderThread;

    mutable QMutex m_nodeMutex;
    QMap<uint8_t, NodeInfo> m_nodes;

    QMutex m_canMutex;
    BatchReadState m_batchReadState = kBatchRead_Idle;

    std::deque<CanCmdItem>  m_cmdQueue;
    std::mutex              m_cmdQueueMutex;
    std::condition_variable m_cmdCv;

    // 接收缓冲区
    mutable std::mutex      m_rxBufferMutex;
    std::deque<can_frame>   m_rxBuffer;
    std::atomic<size_t>     m_rxBufferLimit{20000};

    // ========== 【新增】TPDO 节流相关 ==========
    std::atomic<int> m_tpdoEmitIntervalMs_{20};   // 默认 50Hz
    // 注意：这两个时间点只在接收线程里访问，不需要加锁
    std::chrono::steady_clock::time_point m_lastTpdo2Emit_{};
    std::chrono::steady_clock::time_point m_lastTpdo3Emit_{};

    std::atomic<bool> stop_nmt_read_{false};
};

#endif // _CAN_MANAGER_H_