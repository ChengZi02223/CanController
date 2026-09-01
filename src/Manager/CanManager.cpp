#include "CanManager.h"
#include "Utils.h"


CanManager::CanManager(QObject *parent)
    : QObject(parent), m_running(false){}

CanManager::~CanManager() {
    Stop();
}

// ---------- 启动/停止 ----------
void CanManager::Start() {
    if (m_running) return;
    m_running = true;
    m_receiverThread = std::thread(&CanManager::ReceiverLoop, this);
}

void CanManager::Stop() {
    if (!m_running) return;
    m_running = false;
    if (m_receiverThread.joinable()) m_receiverThread.join();
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

void CanManager::ReceiverLoop() {
    while (m_running) {
        uint32_t cobId;
        std::vector<uint8_t> data;
        if (ReceiveFrame(cobId, data, 100)) {
            ProcessFrame(cobId, data);
        }
    }
}

static int text_index_ = 0;

// ---------- 底层CAN（线程安全） ----------
bool CanManager::SendFrame(uint32_t id, const std::vector<uint8_t>& data) {
    PrintCmd(id, data);
#ifdef ON_TEST_MODE
    m_batchReadState = kBatchRead_WaitAck;
    return true;
#endif
    if (data.empty() || data.size() > 8) return false;
    can_frame frame;
    frame.can_id = id;
    frame.can_dlc = static_cast<uint8_t>(data.size());
    memset(frame.data, 0, 8);
    std::copy(data.begin(), data.end(), frame.data);
    // 若 CanDriver 非线程安全，可在此加锁
    // QMutexLocker locker(&m_canMutex);
    return CanDriver::GetInstance()->send(frame);
}

bool CanManager::ReceiveFrame(uint32_t& id, std::vector<uint8_t>& data, int timeoutMs) {
#ifdef ON_TEST_MODE
text_index_ = text_index_ >= kBatchReadTestFrames.size() ? (int)kBatchReadTestFrames.size() - 1 : text_index_;
    auto test_frame = kBatchReadTestFrames[text_index_];
    id = test_frame.cobId;
    data.assign(test_frame.data, test_frame.data + 8);
    PrintCmd(id, data, "Receive: ");
    text_index_ ++;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    return true;
#endif
    can_frame frame{};
    if (!CanDriver::GetInstance()->receive(frame, timeoutMs)) {
        return false;
    }
    id = frame.can_id;
    data.assign(frame.data, frame.data + frame.can_dlc);
    return true;
}

// ---------- 帧处理 ----------
void CanManager::ProcessFrame(uint32_t cobId, const std::vector<uint8_t>& data) {
    // uint8_t nodeId = cobId & 0x7F;
    // uint32_t base = cobId & 0x780;

    // if (base == (HEART_COB_ID(0) & 0x780)) {           // 心跳 0x700
    //     if (data.size() >= 1) {
    //         ProcessHeartbeat(nodeId, data[0]);
    //     }
    // } else if (base == (SDO_TX_BASE_COB_ID(0) & 0x780)) { // SDO 响应 0x580
    //     processSdoResponse(nodeId, data);
    // } else if (cobId >= 0x80 && cobId < 0x100 && data.size() >= 2) { // EMCY
    //     processEmergency(nodeId, data);
    // } else if (base == (RPDO1_COB_ID(0) & 0x780) ||
    //            base == (RPDO2_COB_ID(0) & 0x780) ||
    //            base == (RPDO3_COB_ID(0) & 0x780)) {
    //     processTpdo(cobId, data);
    // } else {
    //     // 其他忽略
    // }
    bool isEndFrame = false;
    bool isNormal4C0Frame = false;
    std::vector<uint8_t> frameDataCopy;

    {
        QMutexLocker lock(&m_canMutex); // 锁只在这个代码块生效，出大括号自动释放
        PrintCmd(cobId, data, "Process: ");

        // ==========分支1：收到0x5C0应答帧==========
        if(cobId == 0x5C0) {
            if(m_batchReadState == kBatchRead_WaitAck) {
                qDebug()<<"[CanManager]收到0x5C0应答，开始接收0x4C0批量数据";
                m_batchReadState = kBatchRead_Receiving;
            }
            return;
        }

        // ==========分支2：收到0x4C0批量参数帧==========
        if(cobId == 0x4C0) {
            if(m_batchReadState != kBatchRead_Receiving) {
                return;
            }
            // 判断byte0为0xFF：接收结束帧
            if(!data.empty() && data[0] == 0xFF) {
                qDebug()<<"[CanManager]收到0x4C0结束帧(byte0=0xFF)，批量读取完成";
                m_batchReadState = kBatchRead_Idle;
                isEndFrame = true;
                m_running = false;
                return;
            }
            // 普通数据帧，拷贝数据副本，出锁之后解析
            isNormal4C0Frame = true;
            frameDataCopy = data;
        }
    } // 🔒离开作用域，m_canMutex自动释放！！！

    // ---------------- 下面全部不在锁保护内执行 ----------------
    if(isNormal4C0Frame) {
        Parse0x4C0Frame(frameDataCopy);
    }

    // 其他CANid原有逻辑保留
}

// 解析普通0x4C0数据帧（结束帧不会进入这里）
void CanManager::Parse0x4C0Frame(const std::vector<uint8_t>& data)
{
    if(data.size() < 8) {
        qDebug()<<"[Parse0x4C0Frame]数据长度不足8字节";
        return;
    }
    // byte0现在不再是结束标记，可以正常使用
    uint16_t indexRaw = (static_cast<uint16_t>(data[2]) << 8) | static_cast<uint16_t>(data[1]);
    uint8_t subIndexRaw = data[3];

    QString strIndex = QString("0x%1").arg(indexRaw,4,16,QChar('0')).toUpper();
    QString strSubIndex;
    if(subIndexRaw == 0x00) {
        strSubIndex = "--";
    } else {
        strSubIndex = QString("0x%1").arg(subIndexRaw,2,16,QChar('0')).toUpper();
    }

    uint32_t valueRaw = (static_cast<uint32_t>(data[7])<<24)
                        | (static_cast<uint32_t>(data[6])<<16)
                        | (static_cast<uint32_t>(data[5])<<8)
                        | static_cast<uint32_t>(data[4]);
    QString strValue = QString::number(valueRaw);

    // qDebug()<<"[Parse0x4C0Frame] index:"<<strIndex<<" sub:"<<strSubIndex<<" value:"<<strValue;
    emit SendRowValue(strValue, strIndex, strSubIndex);
}