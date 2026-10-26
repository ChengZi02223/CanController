#include "CanManager.h"
#include "Utils.h"
#include "can_cmd.h"

#define HeartTimeOut 2000
#define CmdCheckTimeOut 2000
namespace {
// 命令队列长度上限：防止队列无限膨胀（丢最旧的）
constexpr size_t kMaxQueueLen = 100;
// TX 满时重发的退避间隔（5ms 起步指数翻倍，封顶到此值）
constexpr int kStartSendBackoffMs = 2;
constexpr int kMaxSendBackoffMs = 20;
// 连续 TX 满超过该时长打一条日志（设备可能未应答）
constexpr long long kTxStallLogMs = 2000;
// 周期 02 40 的兜底时长：心跳丢失时最多重发这么久，防止无限占用总线
constexpr long long kNmtRepeatMaxMs = 1500;

long long SteadyNowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
} // namespace

CanManager::CanManager(QObject *parent)
    : QObject(parent), m_running(false) {
        receive_time_= new QTime();
    }

CanManager::~CanManager() {
    Stop();
}

// ---------- 启动/停止（现在管理两个线程） ----------
void CanManager::Start() {
    if (m_running) return;
    ResetHeartbeatState();   // 【新增】清掉上一次运行遗留的超时状态
    m_running = true;

    // 接收线程
    m_receiverThread = std::thread(&CanManager::ReceiverLoop, this);
    // 发送线程
    m_senderThread   = std::thread(&CanManager::SenderLoop, this);
}

void CanManager::Stop() {
    if (!m_running) return;
    m_running = false;

    // 唤醒可能正在 wait 的发送线程
    m_cmdCv.notify_all();

    if (m_receiverThread.joinable()) m_receiverThread.join();
    if (m_senderThread.joinable())   m_senderThread.join();
}

// ---------- 节点管理 ----------
bool CanManager::AddNode(uint8_t nodeId, uint32_t heartbeatTimeoutMs) {
    QMutexLocker locker(&m_nodeMutex);
    if (m_nodes.contains(nodeId)) return false;
    NodeInfo info;
    info.nodeId = nodeId;
    info.nmtState = HEART_STATE_PER_OPT;
    info.lastHeartbeatTime = QDateTime::currentMSecsSinceEpoch();
    info.heartbeatTimeout = heartbeatTimeoutMs;
    info.heartbeatEnabled = (heartbeatTimeoutMs > 0);
    m_nodes[nodeId] = info;
    return true;
}

bool CanManager::RemoveNode(uint8_t nodeId) {
    QMutexLocker locker(&m_nodeMutex);
    return m_nodes.remove(nodeId) > 0;
}

void CanManager::SetHeartbeatTimeout(uint8_t nodeId, uint32_t timeoutMs) {
    QMutexLocker locker(&m_nodeMutex);
    auto it = m_nodes.find(nodeId);
    if (it != m_nodes.end()) {
        it->heartbeatTimeout = timeoutMs;
        it->heartbeatEnabled = (timeoutMs > 0);
    }
}

uint64_t CanManager::BeginEpoch() {
    const uint64_t e = m_epoch_.fetch_add(1, std::memory_order_acq_rel) + 1;
    m_minEpoch_.store(e, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lk(m_cmdQueueMutex);
        m_cmdQueue.clear();      // 丢弃上一阶段残留，让新阶段命令立刻排到最前
    }
    m_cmdCv.notify_all();
    return e;
}

// ========== 【新增】命令栈接口 ==========
void CanManager::PushCommand(uint32_t cobId, const std::vector<uint8_t>& cmd, bool answer, uint32_t re_id) {
    PrintCmd(cobId, cmd, "PushCommand");
    if(!CanDriver::GetInstance()->IsInitialized()){
        return;
    }
    PushCommand(CanCmdItem(cobId, cmd, answer, re_id));
}

void CanManager::PushCommand(const CanCmdItem& item) {
    PrintCmd(item.cobId, item.cmd, "PushCommand");
    if(!CanDriver::GetInstance()->IsInitialized()){
        return;
    }
    {
        std::lock_guard<std::mutex> lk(m_cmdQueueMutex);
        CanCmdItem it = item;
        it.epoch = m_epoch_.load(std::memory_order_acquire);
        if (it.latest_value) {
            for (auto q = m_cmdQueue.begin(); q != m_cmdQueue.end(); ) {
                if (q->cobId == it.cobId) {
                    q = m_cmdQueue.erase(q);
                } else {
                    ++q;
                }
            }
        }        
        if (m_cmdQueue.size() >= kMaxQueueLen) {
            m_cmdQueue.pop_back();
        }
        PrintCmd(it.cobId, it.cmd, "PushCommand");
        m_cmdQueue.push_front(it);
    }
    m_cmdCv.notify_one();
}

void CanManager::PushCommands(const std::vector<CanCmdItem>& items) {
    if(items.empty() || !CanDriver::GetInstance()->IsInitialized()) {
        return;
    }{
        std::lock_guard<std::mutex> lk(m_cmdQueueMutex);
        const uint64_t ep = m_epoch_.load(std::memory_order_acquire);
        for (auto rit = items.rbegin(); rit != items.rend(); ++rit) {
            CanCmdItem it = *rit;
            it.epoch = ep;
            if (it.latest_value) {
                for (auto q = m_cmdQueue.begin(); q != m_cmdQueue.end(); ) {
                    if (q->cobId == it.cobId) {
                        q = m_cmdQueue.erase(q);
                    } else {
                        ++q;
                    }
                }
            }
            if (m_cmdQueue.size() >= kMaxQueueLen) {
                m_cmdQueue.pop_back();
            }
            PrintCmd(it.cobId, it.cmd, "PushCommands");
            m_cmdQueue.push_front(it);
        }        
    }
    m_cmdCv.notify_one();
}

void CanManager::ClearCommands() {
    std::lock_guard<std::mutex> lk(m_cmdQueueMutex);
    // while (!m_cmdQueue.empty()) m_cmdQueue.pop();
    m_cmdQueue.clear();
}

size_t CanManager::CommandCount() {
    std::lock_guard<std::mutex> lk(m_cmdQueueMutex);
    return m_cmdQueue.size();
}

// ---------- 接收缓冲区 ----------
void CanManager::EnqueueReceiveFrame(const can_frame& frame) {
    std::lock_guard<std::mutex> lk(m_rxBufferMutex);
    size_t limit = m_rxBufferLimit.load();
    if (limit > 0 && m_rxBuffer.size() >= limit) {
        // 丢最旧的，防止缓冲区无限膨胀
        m_rxBuffer.pop_front();
    }
    m_rxBuffer.push_back(frame);
}

void CanManager::TakeReceiveBuffer(std::vector<can_frame>& out, size_t maxBatch) {
    std::lock_guard<std::mutex> lk(m_rxBufferMutex);
    size_t n = std::min(maxBatch, m_rxBuffer.size());
    out.reserve(out.size() + n);
    for (size_t i = 0; i < n; ++i) {
        out.push_back(m_rxBuffer.front());
        m_rxBuffer.pop_front();
    }
}

size_t CanManager::ReceiveBufferSize() const {
    std::lock_guard<std::mutex> lk(m_rxBufferMutex);
    return m_rxBuffer.size();
}

void CanManager::ClearReceiveBuffer() {
    std::lock_guard<std::mutex> lk(m_rxBufferMutex);
    m_rxBuffer.clear();
}

void CanManager::SetReceiveBufferLimit(size_t limit) {
    m_rxBufferLimit.store(limit);
    if (limit > 0) {
        std::lock_guard<std::mutex> lk(m_rxBufferMutex);
        while (m_rxBuffer.size() > limit) {
            m_rxBuffer.pop_front();
        }
    }
}
// ---------- 接收线程 ----------
void CanManager::ReceiverLoop() {
    long long lastHeartCheck = SteadyNowMs();   // 【新增】心跳轮询计时
    while (m_running) {
#ifndef ON_TEST_MODE
        if (!CanDriver::GetInstance()->IsInitialized()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
#endif
        can_frame frame{};
        if (ReceiveFrame(frame)) {
            ProcessFrame(frame);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        // 【新增】心跳超时检查：无收包时 ReceiveFrame 会在 100ms 后返回 false，
        // 这里不依赖"收到心跳"这个事件，掉电（完全没有帧）也能按时报警。
        long long nowMs = SteadyNowMs();
        if (nowMs - lastHeartCheck >= kHeartbeatCheckIntervalMs) {
            lastHeartCheck = nowMs;
            CheckHeartbeatTimeout();
        }
    }
}

// 【新增】心跳超时检查：只看"距离上次心跳过了多久"，与是否收到新帧无关。
// 前提：必须先"建立过通信"（收到过至少一帧心跳）。软件刚打开、CAN 未初始化、
// 设备本来就没上电时 heart_online_ 为 false，这里直接返回，绝不弹窗。
// 一旦建立过通信，之后掉电超过 HeartTimeOut 就报警（只报一次）。
void CanManager::ResetHeartbeatState() {
    heart_last_ms_  = 0;
    heart_alarm_on_ = false;
    heart_online_   = false;   // 连接成功并收到心跳前，不做超时检测
}

void CanManager::CheckHeartbeatTimeout() {
    if (!heart_online_) {
        return;    // 从未建立通信：不检测、不报警（避免软件一启动就弹窗）
    }
    const long long silentMs = SteadyNowMs() - heart_last_ms_;
    if (silentMs > HeartTimeOut && !heart_alarm_on_) {   // 上升沿才报，不重复弹窗
        heart_alarm_on_ = true;
        emit SendWarningMessage(
            QString("心跳包超时告警：已超过 %1 毫秒未收到心跳包，通信异常！").arg(silentMs));
    }
    // 未超时不做任何事：heart_alarm_on_ 由收到心跳时清除
}

// ---------- 发送线程 ----------
void CanManager::SenderLoop() {
    auto lastNmtCloseSend = std::chrono::steady_clock::now();
    const std::chrono::milliseconds nmtSendInterval{50}; // 50ms发一次，可调整
    while (m_running) {
        // 【新增】CanDriver 未 ready 时什么都不做，也不从队列取命令
        if (!CanDriver::GetInstance()->IsInitialized()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        CanCmdItem item;
        bool hasItem = false;

        {
            std::unique_lock<std::mutex> lk(m_cmdQueueMutex);
            // 栈非空或停止时才醒来；否则最多等 100ms（避免长时间睡死）
            m_cmdCv.wait_for(lk, std::chrono::milliseconds(100), [this]() {
                return !m_cmdQueue.empty() || !m_running.load();
            });

            if (!m_running) break;

            if (!m_cmdQueue.empty()) {
                item = m_cmdQueue.front();   // 取栈顶
                m_cmdQueue.pop_front();            // 出栈
                hasItem = true;
            }
        }

        if (hasItem) {
            if (item.epoch < m_minEpoch_.load(std::memory_order_acquire)) {
                continue;
            }
            // 再次确认驱动就绪，防止在 wait 期间驱动被 close
            if (!CanDriver::GetInstance()->IsInitialized()) {
                // 把命令塞回队列（放回头部，保持原顺序）
                std::lock_guard<std::mutex> lk(m_cmdQueueMutex);
                m_cmdQueue.push_front(item);
                continue;
            }

            // ===== TX 满：轮转重发，保证所有命令最终都发出去，且不堵新命令 =====
            // CAN_Write 在 TX 队列满时立即失败（设备忙/未应答）。此时把失败的
            // 命令放回【队尾】轮转——排在前面的（往往是阶段边界上更新的命令）
            // 先发，避免旧帧把新命令堵住几秒、把曲线时序挤乱。
            // 命令始终留在发送线程手里不回驱动排队，PCAN 驱动队列里最多
            // 只有当前这一条，不会积压出一串"缓存"旧帧。
            if(item.wait_for_answer) {
                check_answer_ = true;
                curr_check_id_ = item.receive_id;
                receive_time_->start();
            }
            if (!CanDriver::GetInstance()->SendCmd(item.cobId, item.cmd, kCmdTimeOut)) {
                if (item.allow_retry) {
                    {
                        std::lock_guard<std::mutex> lk(m_cmdQueueMutex);
                        if (m_cmdQueue.size() >= kMaxQueueLen) {
                            m_cmdQueue.pop_back();
                        }
                        m_cmdQueue.push_back(item);
                    }
                    if (tx_first_fail_ms_ == 0) {
                        tx_first_fail_ms_ = SteadyNowMs();
                    } else if (!tx_stall_logged_ &&
                               SteadyNowMs() - tx_first_fail_ms_ >= kTxStallLogMs) {
                        std::cerr << "[CanManager] TX 连续 " << kTxStallLogMs
                                  << "ms 写入失败，设备可能未应答（总线拥塞/掉线）" << std::endl;
                        tx_stall_logged_ = true;
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(tx_backoff_ms_));
                if (tx_backoff_ms_ < kMaxSendBackoffMs) {
                    tx_backoff_ms_ *= 2;
                }
            } else {
                tx_backoff_ms_   = kStartSendBackoffMs;
                tx_first_fail_ms_ = 0;
                tx_stall_logged_  = false;
            }
        }

        if (stop_nmt_read_.load()) {
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastNmtCloseSend) >= nmtSendInterval) {
                lastNmtCloseSend = now;
                // 兜底超时：心跳丢失时最多重发 kNmtRepeatMaxMs，防止 02 40 无限占用总线
                if (SteadyNowMs() - nmt_repeat_start_ms_.load() >= kNmtRepeatMaxMs) {
                    stop_nmt_read_ = false;
                } else {
                    // 合并入队：队列里最多保留一条 02 40 —— 既不会残留堆积，
                    // 也保证每轮循环仍只发一帧，不额外拖慢其它命令
                    bool exists = false;
                    {
                        std::lock_guard<std::mutex> lk(m_cmdQueueMutex);
                        for (const auto& it : m_cmdQueue) {
                            if (it.cobId == NMT_COB_ID && it.cmd == NMT_CLOSE_READ_CMD) {
                                exists = true;
                                break;
                            }
                        }
                    }
                    if (!exists) {
                        PushCommand(NMT_COB_ID, NMT_CLOSE_READ_CMD);
                    }
                }
            }
        }
    }
}

static int text_index_ = 0;
#ifdef ON_TEST_MODE
void CanManager::OnStartReciveTest() {
    onTestMode_ = true;
}
#endif

bool CanManager::ReceiveFrame(can_frame &frame) {
#ifndef ON_TEST_MODE
    return CanDriver::GetInstance()->receive(frame, 100);
#else
    if(!onTestMode_){
        return false;
    }
    // if(text_index_ >= kTestWarningFrames.size()){
    //     text_index_ = 0;
    //     return false;
    // }
    // auto test_frame = kTestWarningFrames[text_index_];
    auto test_frame = kHeartTestCmd[1];
    frame.can_id = test_frame.cobId;
    frame.can_dlc = 8;
    std::copy(test_frame.data, test_frame.data + 8, frame.data);
    text_index_ ++;
    onTestMode_ = false;
    return true;
#endif
}

void CanManager::OnStopNMTRead() {
    if(!CanDriver::GetInstance()->IsInitialized()){
        return;
    }
    nmt_repeat_start_ms_ = SteadyNowMs();
    stop_nmt_read_ = true;
}

// ---------- 帧处理 ----------
void CanManager::ProcessFrame(const can_frame &frame) {
    // 1) 全量帧进缓冲区
    EnqueueReceiveFrame(frame);

    int dlc      = frame.can_dlc;
    uint32_t id  = frame.can_id;

    auto time_trval = receive_time_->elapsed();
    if(check_answer_ && id == curr_check_id_ && time_trval > CmdCheckTimeOut){
        QString msg = QString("指令执行已超时：%1 毫秒，执行失败！").arg(time_trval);
        emit SendWarningMessage(msg); 
        check_answer_ = false;
    }

    if (id == GetHeartCobId()) {
        if(dlc == 1) {
            if(frame.data[0] == 0x04) {
                // 收到0x04，停止持续发送NMT_CLOSE_READ_CMD
                stop_nmt_read_ = false;
            } 
        }
        // 【修改】心跳只负责"刷新时间戳 + 清除告警态"，超时判断交给
        // CheckHeartbeatTimeout() 轮询完成（掉电时收不到帧也能报警）
        // heart_online_ 置 true 即"通信已建立"，从这一刻起才允许超时检测
        heart_last_ms_  = SteadyNowMs();
        heart_online_   = true;
        heart_alarm_on_ = false;   // 已恢复，允许下次超时再次报警

        // 心跳处理 ...
    } else if (id == 0x5C0) {
        // SDO 响应：低频，保持原样逐帧 emit
        // PrintCanFrame(frame, "rrrrrr: ");
        if (CheckAnswerHead(frame.data, RESPONSE_READ_TO_TABLE)) {//读取参数到表格
            emit SendStartLoadToTable(frame);
        } else if(CheckAnswerHead(frame.data, RESPONSE_SAVETO_EEPROM_CMD)) {//保存到EEPROM
            emit SendSaveToEEPROMDone();
        } else if(CheckAnswerHead(frame.data, RESPONSE_SAVE_DEFAULT_VALUE)) {//保存默认参数
            emit SendSaveToDefaultDone();
        }
    } else if (id == 0x4C0) {
        emit SendReadFromEPROM(frame);
    } else if (id == 0x2C0 && dlc == 8) {
        // ========== 【修改】TPDO2 节流 ==========
        const int interval = m_tpdoEmitIntervalMs_.load();
        const auto now = std::chrono::steady_clock::now();
        if (interval <= 0 ||
            std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastTpdo2Emit_).count() >= interval) {
            m_lastTpdo2Emit_ = now;
            emit SendTpdo2Position(frame);
        }
    } else if (id == 0x3C0 && dlc == 8) {
        // ========== 【修改】TPDO3 节流 ==========
        const int interval = m_tpdoEmitIntervalMs_.load();
        const auto now = std::chrono::steady_clock::now();
        if (interval <= 0 ||
            std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastTpdo3Emit_).count() >= interval) {
            m_lastTpdo3Emit_ = now;
            emit SendTpdo3Current(frame);
        }
    } else if (id == GetWarningCobId()) {
        HandleWarningFrame(frame.data);
    }
}

void CanManager::HandleWarningFrame(const uint8_t data[8]) {
    emit warningFrameReceived(QByteArray(reinterpret_cast<const char*>(data), 8));
}