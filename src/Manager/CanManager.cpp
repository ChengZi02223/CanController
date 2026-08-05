#include "CanManager.h"


CanManager::CanManager(QObject *parent)
    : QObject(parent), m_running(false){}

CanManager::~CanManager() {
    Stop();
    CanDriver::GetInstance()->close();
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

// ---------- 底层CAN（线程安全） ----------
bool CanManager::SendFrame(uint32_t id, const std::vector<uint8_t>& data) {
    if (data.empty() || data.size() > 8) return false;
    can_frame frame;
    frame.can_id = id;
    frame.can_dlc = static_cast<uint8_t>(data.size());
    memset(frame.data, 0, 8);
    std::copy(data.begin(), data.end(), frame.data);
    // 若 CanDriver 非线程安全，可在此加锁
    QMutexLocker locker(&m_canMutex);
    return CanDriver::GetInstance()->send(frame);
}

bool CanManager::ReceiveFrame(uint32_t& id, std::vector<uint8_t>& data, int timeoutMs) {
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
}
