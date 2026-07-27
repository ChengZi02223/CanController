#include "CmdManager.h"
#include "QtWidgets.h"

CmdManager::CmdManager(const std::string& deviceName, uint32_t baudrate, QObject *parent)
    : QObject(parent), m_running(false), m_sdoTimeoutMs(1000)
{
    if (!m_canDriver.init(deviceName, baudrate)) {
        qCritical() << "CAN driver initialization failed!";
    }
    connect(&m_heartbeatTimer, &QTimer::timeout, this, &CmdManager::onHeartbeatTimer);
}

CmdManager::~CmdManager() {
    stop();
    m_canDriver.close();
}

void CmdManager::start() {
    if (m_running) return;
    m_running = true;

    QThread *thread = new QThread(this);
    connect(thread, &QThread::started, [this]() {
        receiverLoop();
    });
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();

    m_heartbeatTimer.start(100);
}

void CmdManager::stop() {
    if (!m_running) return;
    m_running = false;
    m_heartbeatTimer.stop();
}

bool CmdManager::addNode(uint8_t nodeId, uint32_t heartbeatTimeoutMs) {
    QMutexLocker locker(&m_nodeMutex);
    if (m_nodes.contains(nodeId)) return false;
    NodeInfo info;
    info.nodeId = nodeId;
    info.nmtState = HEART_STATE_PER_OPT; // 默认预操作
    info.lastHeartbeatTime = QDateTime::currentMSecsSinceEpoch();
    info.heartbeatTimeout = heartbeatTimeoutMs;
    info.heartbeatEnabled = (heartbeatTimeoutMs > 0);
    m_nodes[nodeId] = info;
    return true;
}

bool CmdManager::removeNode(uint8_t nodeId) {
    QMutexLocker locker(&m_nodeMutex);
    return m_nodes.remove(nodeId) > 0;
}

void CmdManager::setHeartbeatTimeout(uint8_t nodeId, uint32_t timeoutMs) {
    QMutexLocker locker(&m_nodeMutex);
    auto it = m_nodes.find(nodeId);
    if (it != m_nodes.end()) {
        it->heartbeatTimeout = timeoutMs;
        it->heartbeatEnabled = (timeoutMs > 0);
    }
}

void CmdManager::sendNmt(uint8_t nodeId, uint8_t cmd) {
    std::vector<uint8_t> data = { cmd, nodeId };
    if (!sendFrame(NMT_COB_ID, data)) {
        qWarning() << "sendNmt failed";
    }
}

void CmdManager::sdoRead(uint8_t nodeId, uint16_t index, uint8_t subIndex, SdoCallback callback) {
    std::vector<uint8_t> data(8, 0);
    data[0] = SDO_CMD_UPLOAD_REQUEST;
    data[1] = index & 0xFF;
    data[2] = (index >> 8) & 0xFF;
    data[3] = subIndex;

    uint32_t key = makeTransactionKey(nodeId, index);
    {
        QMutexLocker locker(&m_sdoMutex);
        auto it = m_sdoTransactions.find(key);
        if (it != m_sdoTransactions.end()) {
            qWarning() << "SDO transaction already pending for index 0x" << hex << index;
            if (it->callback) it->callback(false, {});
            m_sdoTransactions.erase(it);
        }
        SdoTransaction trans;
        trans.nodeId = nodeId;
        trans.index = index;
        trans.subIndex = subIndex;
        trans.callback = callback;
        trans.startTime = QDateTime::currentMSecsSinceEpoch();
        trans.isWrite = false;
        trans.toggle = 0;
        m_sdoTransactions[key] = trans;
    }

    uint32_t cobId = SDO_RX_BASE_COB_ID(nodeId);
    if (!sendFrame(cobId, data)) {
        qWarning() << "sdoRead send failed";
        QMutexLocker locker(&m_sdoMutex);
        auto it = m_sdoTransactions.find(key);
        if (it != m_sdoTransactions.end()) {
            if (it->callback) it->callback(false, {});
            m_sdoTransactions.erase(it);
        }
    }
}

void CmdManager::sdoWrite(uint8_t nodeId, uint16_t index, uint8_t subIndex, const std::vector<uint8_t>& data, SdoCallback callback) {
    if (data.size() > 4) {
        qWarning() << "SDO write with data >4 bytes not implemented (use segmented)";
        if (callback) callback(false, {});
        return;
    }

    uint8_t cmd;
    switch (data.size()) {
        case 1: cmd = SDO_CMD_DOWNLOAD_1_BYTE; break;
        case 2: cmd = SDO_CMD_DOWNLOAD_2_BYTE; break;
        case 3: cmd = SDO_CMD_DOWNLOAD_3_BYTE; break;
        case 4: cmd = SDO_CMD_DOWNLOAD_4_BYTE; break;
        default: cmd = SDO_CMD_DOWNLOAD_1_BYTE; break;
    }

    std::vector<uint8_t> frame(8, 0);
    frame[0] = cmd;
    frame[1] = index & 0xFF;
    frame[2] = (index >> 8) & 0xFF;
    frame[3] = subIndex;
    std::copy(data.begin(), data.end(), frame.begin() + 4);

    uint32_t key = makeTransactionKey(nodeId, index);
    {
        QMutexLocker locker(&m_sdoMutex);
        auto it = m_sdoTransactions.find(key);
        if (it != m_sdoTransactions.end()) {
            if (it->callback) it->callback(false, {});
            m_sdoTransactions.erase(it);
        }
        SdoTransaction trans;
        trans.nodeId = nodeId;
        trans.index = index;
        trans.subIndex = subIndex;
        trans.callback = callback;
        trans.startTime = QDateTime::currentMSecsSinceEpoch();
        trans.isWrite = true;
        trans.toggle = 0;
        m_sdoTransactions[key] = trans;
    }

    uint32_t cobId = SDO_RX_BASE_COB_ID(nodeId);
    if (!sendFrame(cobId, frame)) {
        qWarning() << "sdoWrite send failed";
        QMutexLocker locker(&m_sdoMutex);
        auto it = m_sdoTransactions.find(key);
        if (it != m_sdoTransactions.end()) {
            if (it->callback) it->callback(false, {});
            m_sdoTransactions.erase(it);
        }
    }
}

bool CmdManager::sendRPDO(uint8_t nodeId, uint8_t pdoNum, const std::vector<uint8_t>& data) {
    if (data.size() > 8) {
        qWarning() << "RPDO data exceeds 8 bytes";
        return false;
    }
    uint32_t cobId;
    switch (pdoNum) {
        case 1: cobId = RPDO1_COB_ID(nodeId); break;
        case 2: cobId = RPDO2_COB_ID(nodeId); break;
        case 3: cobId = RPDO3_COB_ID(nodeId); break;
        default: return false;
    }
    std::vector<uint8_t> frame = data;
    frame.resize(8, 0);
    return sendFrame(cobId, frame);
}

void CmdManager::registerTpdoCallback(uint32_t cobId, std::function<void(const std::vector<uint8_t>&)> callback) {
    connect(this, &CmdManager::tpdoReceived, [cobId, callback](uint32_t receivedCob, const std::vector<uint8_t>& data){
        if (receivedCob == cobId && callback) callback(data);
    });
}

void CmdManager::onHeartbeatTimer() {
    uint64_t now = QDateTime::currentMSecsSinceEpoch();
    QMutexLocker locker(&m_nodeMutex);
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        NodeInfo& info = it.value();
        if (!info.heartbeatEnabled) continue;
        if (now - info.lastHeartbeatTime > info.heartbeatTimeout) {
            emit heartbeatTimeout(info.nodeId);
        }
    }
    checkSdoTimeouts();
}

void CmdManager::checkSdoTimeouts() {
    uint64_t now = QDateTime::currentMSecsSinceEpoch();
    QMutexLocker locker(&m_sdoMutex);
    for (auto it = m_sdoTransactions.begin(); it != m_sdoTransactions.end(); ) {
        if (now - it->startTime > m_sdoTimeoutMs) {
            qWarning() << "SDO timeout for node" << it->nodeId << "index" << it->index;
            if (it->callback) it->callback(false, {});
            it = m_sdoTransactions.erase(it);
        } else {
            ++it;
        }
    }
}

// ---------- 底层CAN封装 ----------
bool CmdManager::sendFrame(uint32_t id, const std::vector<uint8_t>& data) {
    if (data.empty() || data.size() > 8) return false;
    can_frame frame;
    frame.can_id = id & 0x7FF;
    frame.can_dlc = static_cast<uint8_t>(data.size());
    memset(frame.data, 0, 8);
    std::copy(data.begin(), data.end(), frame.data);
    return m_canDriver.send(frame);
}

bool CmdManager::receiveFrame(uint32_t& id, std::vector<uint8_t>& data, int timeoutMs) {
    can_frame frame;
    if (!m_canDriver.receive(frame, timeoutMs)) {
        return false;
    }
    id = frame.can_id;
    data.assign(frame.data, frame.data + frame.can_dlc);
    return true;
}

// ---------- 接收线程 ----------
void CmdManager::receiverLoop() {
    while (m_running) {
        uint32_t cobId;
        std::vector<uint8_t> data;
        if (receiveFrame(cobId, data, 100)) {
            processFrame(cobId, data);
        }
    }
}

void CmdManager::processFrame(uint32_t cobId, const std::vector<uint8_t>& data) {
    uint8_t nodeId = cobId & 0x7F;
    uint32_t base = cobId & 0x780;

    if (base == (HEART_COB_ID(0) & 0x780)) { // 心跳基地址 0x700
        if (data.size() >= 1) {
            uint8_t state = data[0];
            processHeartbeat(nodeId, state);
        }
    } else if (base == (SDO_TX_BASE_COB_ID(0) & 0x780)) { // 0x580
        processSdoResponse(nodeId, data);
    } else if (base == (EMCY_COB_ID(0) & 0x780)) { // 0x80? 注意EMCY基址是0x80，但nodeId在低7位，所以基址是0x80
        // 但这里我们判断cobId >= 0x80 && cobId < 0x100，但可能与其他冲突，我们直接判断基址
        // 但EMCY COB-ID = 0x80 + nodeId，所以基址是0x80，但会与SYNC (0x80)冲突，但SYNC无数据且无nodeId，我们通过数据长度判断
        // 简单处理：如果cobId >= 0x80 && cobId < 0x100 且 data长度>=2，认为是EMCY
        if (cobId >= 0x80 && cobId < 0x100 && data.size() >= 2) {
            processEmergency(nodeId, data);
        } else {
            // 可能是SYNC或其他，忽略
        }
    } else if (base == (RPDO1_COB_ID(0) & 0x780) || base == (RPDO2_COB_ID(0) & 0x780) || base == (RPDO3_COB_ID(0) & 0x780)) {
        processTpdo(cobId, data);
    } else {
        // 其他忽略
    }
}

void CmdManager::processHeartbeat(uint8_t nodeId, uint8_t state) {
    QMutexLocker locker(&m_nodeMutex);
    auto it = m_nodes.find(nodeId);
    if (it != m_nodes.end()) {
        it->lastHeartbeatTime = QDateTime::currentMSecsSinceEpoch();
        if (it->nmtState != state) {
            it->nmtState = state;
            emit nmtStateChanged(nodeId, state);
        }
    }
}

void CmdManager::processSdoResponse(uint8_t nodeId, const std::vector<uint8_t>& data) {
    if (data.size() < 4) return;
    uint8_t cmd = data[0];
    uint16_t index = data[1] | (data[2] << 8);
    uint8_t subIdx = data[3];

    // 检查中止
    if ((cmd & 0xE0) == SDO_CMD_ABORT) {
        uint32_t abortCode = 0;
        if (data.size() >= 8) {
            abortCode = data[4] | (data[5]<<8) | (data[6]<<16) | (data[7]<<24);
        }
        qWarning() << "SDO abort, node" << nodeId << "index" << hex << index << "code" << abortCode;
        uint32_t key = makeTransactionKey(nodeId, index);
        QMutexLocker locker(&m_sdoMutex);
        auto it = m_sdoTransactions.find(key);
        if (it != m_sdoTransactions.end()) {
            if (it->callback) it->callback(false, {});
            m_sdoTransactions.erase(it);
        }
        return;
    }

    // 写响应
    if (cmd == SDO_CMD_DOWNLOAD_REQUEST) { // 0x60
        uint32_t key = makeTransactionKey(nodeId, index);
        QMutexLocker locker(&m_sdoMutex);
        auto it = m_sdoTransactions.find(key);
        if (it != m_sdoTransactions.end()) {
            if (it->callback) it->callback(true, {});
            m_sdoTransactions.erase(it);
        }
    }
    // 读响应 (0x43,0x47,0x4B,0x4F)
    else if ((cmd & 0xF0) == 0x40) {
        size_t dataLen = 0;
        if (cmd == SDO_CMD_UPLOAD_4_BYTE) dataLen = 4;
        else if (cmd == SDO_CMD_UPLOAD_3_BYTE) dataLen = 3;
        else if (cmd == SDO_CMD_UPLOAD_2_BYTE) dataLen = 2;
        else if (cmd == SDO_CMD_UPLOAD_1_BYTE) dataLen = 1;
        else {
            qWarning() << "Unknown SDO upload response cmd:" << hex << cmd;
            return;
        }
        std::vector<uint8_t> result(data.begin() + 4, data.begin() + 4 + dataLen);
        uint32_t key = makeTransactionKey(nodeId, index);
        QMutexLocker locker(&m_sdoMutex);
        auto it = m_sdoTransactions.find(key);
        if (it != m_sdoTransactions.end()) {
            if (it->callback) it->callback(true, result);
            m_sdoTransactions.erase(it);
        }
    } else {
        qWarning() << "Unhandled SDO response cmd:" << hex << cmd;
    }
}

void CmdManager::processEmergency(uint8_t nodeId, const std::vector<uint8_t>& data) {
    if (data.size() < 2) return;
    uint16_t errorCode = data[0] | (data[1] << 8);
    std::vector<uint8_t> additional;
    if (data.size() > 2) {
        additional.assign(data.begin() + 2, data.end());
    }
    emit emergencyReceived(nodeId, errorCode, additional);
}

void CmdManager::processTpdo(uint32_t cobId, const std::vector<uint8_t>& data) {
    emit tpdoReceived(cobId, data);
}