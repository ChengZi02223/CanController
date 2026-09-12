#include "CanManager.h"
#include "Utils.h"
#include "can_cmd.h"

namespace {
// 命令队列长度上限：TX 满、发送停滞时防止队列无限膨胀（丢最旧的）
constexpr size_t kMaxQueueLen = 100;
// TX 满时退避重发的间隔上限（5ms 起步指数翻倍，封顶到此值）
constexpr int kMaxSendBackoffMs = 100;
// TX 满时单条命令的总等待耐心：超过该时长仍发不出去，判定设备掉线/不应答，
// 清队列+复位PCAN恢复，避免发送线程被单条命令永久卡死
constexpr long long kSendPatienceMs = 3000;
// 周期 02 40 的兜底时长：心跳丢失时最多重发这么久，防止无限占用总线
constexpr long long kNmtRepeatMaxMs = 1500;

long long SteadyNowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
} // namespace

CanManager::CanManager(QObject *parent)
    : QObject(parent), m_running(false) {}

CanManager::~CanManager() {
    Stop();
}

// ---------- 启动/停止（现在管理两个线程） ----------
void CanManager::Start() {
    if (m_running) return;
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


// ========== 【新增】命令栈接口 ==========
void CanManager::PushCommand(uint32_t cobId, const std::vector<uint8_t>& cmd) {
    if(!CanDriver::GetInstance()->IsInitialized()){
        return;
    }
    {
        std::lock_guard<std::mutex> lk(m_cmdQueueMutex);
        if (m_cmdQueue.size() >= kMaxQueueLen) {
            m_cmdQueue.pop_back();   // 防止队列无限膨胀，丢最旧的
        }
        m_cmdQueue.push_front(CanCmdItem(cobId, cmd));
    }
    m_cmdCv.notify_one();   // 唤醒发送线程，立即出栈发送
}

void CanManager::PushCommand(const CanCmdItem& item) {
    if(!CanDriver::GetInstance()->IsInitialized()){
        return;
    }
    {
        std::lock_guard<std::mutex> lk(m_cmdQueueMutex);
        if (m_cmdQueue.size() >= kMaxQueueLen) {
            m_cmdQueue.pop_back();
        }
        m_cmdQueue.push_front(item);
    }
    m_cmdCv.notify_one();
}

void CanManager::PushCommands(const std::vector<CanCmdItem>& items) {
    if(items.empty() || !CanDriver::GetInstance()->IsInitialized()) {
        return;
    }
    for(auto &item: items) {
        std::lock_guard<std::mutex> lk(m_cmdQueueMutex);
        if (m_cmdQueue.size() >= kMaxQueueLen) {
            m_cmdQueue.pop_back();
        }
        m_cmdQueue.push_front(item);
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
    while (m_running) {
        // 【新增】CanDriver 未 ready 时什么都不做
        if (!CanDriver::GetInstance()->IsInitialized()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        can_frame frame{};
        if (ReceiveFrame(frame)) {
            ProcessFrame(frame);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
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
            // 再次确认驱动就绪，防止在 wait 期间驱动被 close
            if (!CanDriver::GetInstance()->IsInitialized()) {
                // 把命令塞回队列（放回头部，保持原顺序）
                std::lock_guard<std::mutex> lk(m_cmdQueueMutex);
                m_cmdQueue.push_front(item);
                continue;
            }

            // ===== TX 满：指数退避重发，保证这条命令最终发出去 =====
            // CAN_Write 在 TX 队列满时立即失败，说明总线暂时拥塞/设备忙；
            // 此时对同一条命令按 5ms→10ms→20ms→...→100ms 退避持续重试，
            // 发出去之前不碰后续命令（保序），不丢命令、不积压到硬件。
            // 命令始终留在发送线程手里，PCAN 驱动队列里最多只有当前这一条，
            // 设备恢复后收到的是最新意图，不会冒出一串"缓存"旧命令。
            bool sent = false;
            long long first_fail_ms = 0;
            int backoff_ms = 5;
            while (m_running) {
                if (!CanDriver::GetInstance()->IsInitialized()) {
                    break;   // 驱动被关闭，交还给外层处理
                }
                if (CanDriver::GetInstance()->SendCmd(item.cobId, item.cmd, kCmdTimeOut)) {
                    sent = true;
                    break;
                }
                if (first_fail_ms == 0) {
                    first_fail_ms = SteadyNowMs();
                } else if (SteadyNowMs() - first_fail_ms >= kSendPatienceMs) {
                    break;   // 超出耐心阈值，总线大概率异常（设备不应答）
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
                if (backoff_ms < kMaxSendBackoffMs) {
                    backoff_ms *= 2;
                }
            }

            if (!m_running || !CanDriver::GetInstance()->IsInitialized()) {
                // 线程停止或驱动关闭：命令塞回队首，等恢复后继续
                std::lock_guard<std::mutex> lk(m_cmdQueueMutex);
                m_cmdQueue.push_front(item);
                continue;
            }

            if (sent) {
                continue;   // 发送成功，立刻处理下一条
            }

            // 耐心耗尽：设备长时间不应答，卡死在这一条上会永久堵塞发送线程。
            // 清空队列并复位 PCAN（清掉卡在硬件里无限重发的帧），让新命令重新开始
            std::cerr << "[CanManager] TX blocked over " << kSendPatienceMs
                      << "ms (device not ACKing?), purge queue and reset PCAN" << std::endl;
            ClearCommands();
            CanDriver::GetInstance()->HardReset();
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

bool CanManager::ReceiveFrame(can_frame &frame) {
#ifndef ON_TEST_MODE
    return CanDriver::GetInstance()->receive(frame, 100);
#else
    if(text_index_ >= kBatchReadTestFrames.size()){
        text_index_ = 0;
        return false;
    }
    auto test_frame = kBatchReadTestFrames[text_index_];
    frame.can_id = test_frame.cobId;
    frame.can_dlc = 8;
    std::copy(test_frame.data, test_frame.data + 8, frame.data);
    text_index_ ++;
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

    if (id == 0x740) {
        if(dlc == 1) {
            if(frame.data[0] == 0x04) {
                // 收到0x04，停止持续发送NMT_CLOSE_READ_CMD
                stop_nmt_read_ = false;
            } 
        }
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
    }
}