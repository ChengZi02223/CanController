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
#include <QTimer>

#include "CanDriver.h"
#include "CO_CMD.h"
#include "Utils.h"

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

    void PushCommand(uint32_t cobId, const std::vector<uint8_t>& cmd, bool answer = false, uint32_t re_id = 0x00);
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

    void HandleWarningFrame(const uint8_t data[8]);
    uint64_t BeginEpoch();
    uint64_t CurrentEpoch() const { return m_epoch_.load(std::memory_order_acquire); }

signals:
    void SendRowValue(QString value, QString idx, QString sub_idx);

    void SendStartLoadToTable(can_frame frame);
    void SendSaveToEEPROMDone();
    void SendSaveToDefaultDone();

    void SendReadFromEPROM(can_frame frame);

    void SendTpdo2Position(can_frame frame);
    void SendTpdo3Current(can_frame frame);
    void warningFrameReceived(const QByteArray &data);

    void SendWarningMessage(const QString &msg);

public slots:
    void OnStopNMTRead();
#ifdef ON_TEST_MODE
    void OnStartReciveTest();
#endif

public:
    // 主动停止周期性发送 NMT_CLOSE_READ_CMD
    // （收到0x04心跳会自动停；此接口供其它页面在发起新命令前兜底调用，
    //   防止心跳丢失导致周期 02 40 一直占用总线、干扰后续命令）
    void StopNmtCloseRepeat() { stop_nmt_read_ = false; }

private:
    explicit CanManager(QObject *parent = nullptr);
    ~CanManager();

    void ReceiverLoop();
    void SenderLoop();

    bool ReceiveFrame(can_frame &frame);
    void ProcessFrame(const can_frame &frame);

    void EnqueueReceiveFrame(const can_frame& frame);

private:
#ifdef ON_TEST_MODE
    bool onTestMode_ = false;
#endif
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
    // 本轮周期 02 40 的起始时间（ms, steady_clock），用于兜底超时自动停止
    std::atomic<long long> nmt_repeat_start_ms_{0};

    // ===== TX 满/设备未应答时的发送轮转状态（仅发送线程访问） =====
    int tx_backoff_ms_ = 5;                  // 当前退避间隔
    long long tx_first_fail_ms_ = 0;         // 本轮连续失败起点
    bool tx_stall_logged_ = false;           // 本轮是否已打过停顿日志

    QTime *receive_time_ = nullptr;
    uint16_t curr_check_id_ = 0x00;
    bool check_answer_ = false;

    // ========== 【修改】心跳超时检测 ==========
    // 原实现只在"收到心跳"时才比较两次心跳的间隔，掉电后永远收不到帧，
    // 因此超时永远不会触发。改为由接收线程轮询检查"距上次心跳已过多久"。
    // 【再修改】只有在"已经连接成功并收到过心跳"之后才启用检测：
    //   软件刚打开、CAN 还没初始化（或设备本来就没上电）时，不会有任何告警弹窗。
    static constexpr long long kHeartbeatCheckIntervalMs = 100;  // 轮询周期
    long long heart_last_ms_   = 0;      // 最近一次收到心跳的时刻（steady_clock）
    bool      heart_alarm_on_  = false;  // 当前是否已处于超时告警态（防重复弹窗）
    bool      heart_online_    = false;  // 是否已"建立过通信"（收到过心跳）；为 false 时不做任何检测

    void CheckHeartbeatTimeout();        // 由接收线程周期调用
    void ResetHeartbeatState();          // Start() 时重置：首次连接前保持"不检测"

    // ===== 命令纪元（仅发送线程 / Push 接口访问）=====
    std::atomic<uint64_t> m_epoch_{1};      // 下一个可用纪元号
    std::atomic<uint64_t> m_minEpoch_{1};   // 小于该值的命令一律丢弃
};

#endif // _CAN_MANAGER_H_