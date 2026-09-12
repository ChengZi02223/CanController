#include "CanDriver.h"
#include "CO_CMD.h"
#include <iostream>
#include <cstring>
#include <vector>
#include "can_cmd.h"
#include "Utils.h"
#include <QDateTime>
#include <QThread>

CanDriver::CanDriver()
    : handle_(nullptr), isInitialized_(false)
{
}

CanDriver::~CanDriver()
{
    close();
}

// ====================== 新增：自动扫描所有PCAN通道 ======================
std::vector<CanChannelInfo> CanDriver::scanAllChannels()
{
    std::vector<CanChannelInfo> channelList;
    DWORD channelCount = 0;

    // 1. 获取当前电脑挂载的PCAN通道总数，第一个参数固定PCAN_NONEBUS
    TPCANStatus status = CAN_GetValue(
        PCAN_NONEBUS,
        PCAN_ATTACHED_CHANNELS_COUNT,
        &channelCount,
        sizeof(DWORD)
    );
    if (status != PCAN_ERROR_OK || channelCount == 0)
    {
        return channelList; // 无设备
    }

    // 2. 分配缓冲区，读取全部通道详情
    std::vector<TPCANChannelInformation> chanInfos(channelCount);
    status = CAN_GetValue(
        PCAN_NONEBUS,
        PCAN_ATTACHED_CHANNELS,
        chanInfos.data(),
        channelCount * sizeof(TPCANChannelInformation)
    );
    if (status != PCAN_ERROR_OK)
    {
        return channelList;
    }

    // 3. 遍历解析每个通道信息
    for (const auto& info : chanInfos)
    {
        CanChannelInfo item;
        item.handle = info.channel_handle;
        item.hardwareName = info.device_name;

        // 把handle转成通道名字符串（PCAN_USBBUS1 / PCAN_PCIBUS1等）
        char nameBuf[64] = {0};
        TPCANStatus strStatus = CAN_GetValue(
            info.channel_handle,
            PCAN_HARDWARE_NAME,
            nameBuf,
            sizeof(nameBuf)
        );
        item.channelName = std::string(nameBuf);

        // 判断通道状态：可用 / 被占用
        item.isAvailable = (info.channel_condition & PCAN_CHANNEL_AVAILABLE) != 0;
        // 判断是否支持CAN FD
        item.isFDSupport = (info.device_features & FEATURE_FD_CAPABLE) != 0;

        channelList.push_back(item);
    }
    return channelList;
}

// ====================== 改造初始化：直接传入扫描到的handle ======================
bool CanDriver::init(TPCANHandle channelHandle, uint32_t baudrate)
{
    std::lock_guard<std::mutex> lk_tx(m_tx_mtx_);
    std::lock_guard<std::mutex> lk_rx(m_rx_mtx_);
    close_NoLock();   // 调用无锁版本！！不要调用带锁close()！！
    return init_NoLock(channelHandle, baudrate);
}

// 内部无锁，调用者已经持有tx+rx锁
bool CanDriver::init_NoLock(TPCANHandle channelHandle, uint32_t baudrate)
{
    qDebug() << "init_NoLock";
    TPCANBaudrate pcanBaud;
    switch (baudrate) {
        case 125000: pcanBaud = PCAN_BAUD_125K; break;
        case 250000: pcanBaud = PCAN_BAUD_250K; break;
        case 500000: pcanBaud = PCAN_BAUD_500K; break;
        case 1000000: pcanBaud = PCAN_BAUD_1M;   break;
        default:      pcanBaud = PCAN_BAUD_250K; break;
    }
    TPCANStatus status = CAN_Initialize(channelHandle, pcanBaud, 0,0,0);
    if (status != PCAN_ERROR_OK) {
        char errText[256] = {0};
        CAN_GetErrorText(status,0x09,errText);
        std::cerr << "CAN初始化失败:" << errText << " 错误码:0x" << std::hex << status << std::endl;
        return false;
    }
    baudrate_ = baudrate;
    handle_ = reinterpret_cast<void*>(static_cast<uintptr_t>(channelHandle));
    isInitialized_ = true;
    return true;
}

void CanDriver::close()
{
    std::lock_guard<std::mutex> lk_tx(m_tx_mtx_);
    std::lock_guard<std::mutex> lk_rx(m_rx_mtx_);
    close_NoLock();
}

// 内部无锁，调用者必须已经持有tx+rx锁
void CanDriver::close_NoLock()
{
    if (isInitialized_ && handle_) {
        qDebug() << "close_NoLock";
        TPCANHandle pcanHandle = static_cast<TPCANHandle>(reinterpret_cast<uintptr_t>(handle_));
        CAN_Uninitialize(pcanHandle);
        handle_ = nullptr;
        isInitialized_ = false;
    }
}

bool CanDriver::SendCmdWithRetry(uint32_t cobId, const std::vector<uint8_t>& cmd, int maxRetries, int retryIntervalMs) {
    std::lock_guard<std::mutex> lock(m_tx_mtx_);
    for (int i = 0; i < maxRetries; ++i) {
        if (SendCmd_NoLock(cobId, cmd, kCmdTimeOut)) {
            return true;
        }
        if (i < maxRetries - 1) {
            QThread::msleep(retryIntervalMs);
        }
    }
    return false;
}

bool CanDriver::SendCmd_NoLock(const uint32_t cobId, const std::vector<uint8_t>& cmd, int timeout_ms)
{
    PrintCmd(cobId, cmd);
#ifdef ON_TEST_MODE
    return true;
#endif
    can_frame frame{};
    frame.can_id = cobId;
    frame.can_dlc = static_cast<uint8_t>(cmd.size());
    const size_t copyLen = std::min(cmd.size(), sizeof(frame.data));
    std::memcpy(frame.data, cmd.data(), copyLen);
    if(frame.can_id > 0x7FF){
        frame.can_id |= CAN_EFF_FLAG;
    }
    if(!send_NoLock(frame)) {
        return false;
    }
    return true;
}

bool CanDriver::SendCmd(const uint32_t cobId, const std::vector<uint8_t>& cmd, int timeout_ms) {
    std::lock_guard<std::mutex> lock(m_tx_mtx_);
    return SendCmd_NoLock(cobId, cmd, timeout_ms);
}

bool CanDriver::ExecCmd(const uint32_t cobId, const std::vector<uint8_t> cmd, can_frame& response, int timeout_ms) {
    FlushRxBuffer();

    {
        std::lock_guard<std::mutex> lock(m_tx_mtx_);
        if (!SendCmd_NoLock(cobId, cmd, timeout_ms))
        {
            return false;
        }
    } 

    return receive(response, timeout_ms);
}

bool CanDriver::ExecCmd(const uint32_t cobId, const std::vector<uint8_t>& cmd, int timeout_ms) {
    std::lock_guard<std::mutex> lock(m_tx_mtx_);
    return SendCmd_NoLock(cobId, cmd, timeout_ms);
}

bool CanDriver::ExecCmds(const std::vector<CanCmdItem>& cmdList)
{
    // 整个批量发送全程持有互斥锁，保证多条报文连续输出，不被其他ExecCmd抢占
    std::lock_guard<std::mutex> lock(m_tx_mtx_);

    for (const auto& item : cmdList)
    {
        const uint32_t cobId = item.cobId;
        const std::vector<uint8_t>& cmd = item.cmd;

        // 打印指令，复用原有打印逻辑
        PrintCmd(cobId, cmd, "ExecCmds: ");
#ifndef ON_TEST_MODE
        can_frame frame{};
        frame.can_id = cobId;
        // DLC取实际长度，CAN最大8字节
        const size_t copyLen = std::min(cmd.size(), sizeof(frame.data));
        frame.can_dlc = static_cast<uint8_t>(copyLen);
        std::memcpy(frame.data, cmd.data(), copyLen);

        // 扩展帧标志：大于0x7FF开启EFF
        if (frame.can_id > 0x7FF)
        {
            frame.can_id |= CAN_EFF_FLAG;
        }

        // 发送失败直接返回false，不再继续发送剩下的报文
        if (!send_NoLock(frame))
        {
            return false;
        }
#endif
    }

    // 全部报文发送完成
    return true;
}

// ====================== 原有发送函数不变 ======================
bool CanDriver::send(const can_frame& frame)
{
    std::lock_guard<std::mutex> lock(m_tx_mtx_);
    return send_NoLock(frame);
}

bool CanDriver::send_NoLock(const can_frame& frame) {
    if (!isInitialized_ || !handle_) return false;
    if (!CRCCheck(frame)) return false;

    TPCANHandle pcanHandle = static_cast<TPCANHandle>(reinterpret_cast<uintptr_t>(handle_));
    TPCANMsg msg;
    msg.ID = frame.can_id & CAN_EFF_MASK;
    msg.MSGTYPE = (frame.can_id & CAN_EFF_FLAG) ? PCAN_MESSAGE_EXTENDED : PCAN_MESSAGE_STANDARD;
    msg.LEN = frame.can_dlc;
    std::memcpy(msg.DATA, frame.data, 8);

    TPCANStatus status = CAN_Write(pcanHandle, &msg);
    if (status != PCAN_ERROR_OK) {
        // TX 缓冲/队列满：立即返回失败，由上层（发送线程）重试；
        // 这里不能打印日志也不能等待，否则会卡住发送线程、拖慢所有命令
        if (status == PCAN_ERROR_XMTFULL || status == PCAN_ERROR_QXMTFULL) {
            return false;
        }
        std::cerr << "发送失败，错误码: 0x" << std::hex << status << std::endl;
        return false;
    }
    return true;
}
// ====================== 原有接收函数优化：增加错误文本 ======================
bool CanDriver::receive(can_frame& frame, int timeout_ms)
{
    if (!isInitialized_ || !handle_)
    {
        return false;
    }
    TPCANHandle pcanHandle = static_cast<TPCANHandle>(reinterpret_cast<uintptr_t>(handle_));
    TPCANMsg msg;
    TPCANTimestamp ts;
    qint64 start = QDateTime::currentMSecsSinceEpoch();

    do
    {
        TPCANStatus status;
        {
            // 【锁范围极小：只保护一次CAN_Read，读完马上释放！Sleep不在锁内】
            std::lock_guard<std::mutex> lk_rx(m_rx_mtx_);
            status = CAN_Read(pcanHandle, &msg, &ts);
            if (status == PCAN_ERROR_OK)
            {
                frame.can_id = msg.ID;
                frame.can_dlc = msg.LEN;
                memset(frame.data, 0, sizeof(frame.data));
                std::memcpy(frame.data, msg.DATA, msg.LEN);
                return true;
            }
            else if (status != PCAN_ERROR_QRCVEMPTY)
            {
                char errText[256] = {0};
                CAN_GetErrorText(status, 0x09, errText);
                std::cerr << "接收异常:" << errText << " 错误码:0x" << std::hex << status << std::endl;
                return false;
            }
        }
        // 出大括号：rx锁已经释放！！sleep在锁外面！！
        Sleep(2);
    } while ((QDateTime::currentMSecsSinceEpoch() - start) < timeout_ms);

    // 超时
    return false;
}


bool CanDriver::CRCCheck(const can_frame& frame) {
    // 只检测 0x240
    if(frame.can_id != SEND_COB_ID) {
        return true;
    }
    if(frame.can_dlc < 8) {
        return false;
    }
    uint8_t sum = 0;
    for(int i = 0; i < 7; ++i) {
        sum += frame.data[i];
    }
    uint8_t recvSum = frame.data[7];
    return (sum == recvSum);

}

void CanDriver::FlushRxBuffer() {
    can_frame dummy;
    while (receive(dummy, 10)) { /* 读空 */ }
}

// 轻量清理：CAN_Reset 清空驱动的接收+发送FIFO
// 注意：PEAK官方文档明确 —— 已进入硬件缓冲区的帧不会被CAN_Reset删除
bool CanDriver::FlushBuffers() {
    std::lock_guard<std::mutex> lk_tx(m_tx_mtx_);
    std::lock_guard<std::mutex> lk_rx(m_rx_mtx_);
    if (!isInitialized_ || !handle_) return false;

    TPCANHandle h = static_cast<TPCANHandle>(reinterpret_cast<uintptr_t>(handle_));
    TPCANStatus status = CAN_Reset(h);
    if (status != PCAN_ERROR_OK) {
        char errText[256] = {0};
        CAN_GetErrorText(status, 0x09, errText);
        std::cerr << "FlushBuffers失败:" << errText << std::endl;
        return false;
    }
    return true;
}

// 彻底复位：CAN_Uninitialize + CAN_Initialize
// PEAK官方论坛确认：这是唯一能清掉"卡在硬件里无限重发的帧"的方法
bool CanDriver::HardReset() {
    std::lock_guard<std::mutex> lk_tx(m_tx_mtx_);
    std::lock_guard<std::mutex> lk_rx(m_rx_mtx_);
    if (!isInitialized_ || !handle_) return false;

    TPCANHandle h = static_cast<TPCANHandle>(reinterpret_cast<uintptr_t>(handle_));

    // 开启"硬复位"模式：此后 CAN_Reset 会真正复位 CAN 控制器硬件
    BYTE on = PCAN_PARAMETER_ON;
    TPCANStatus st = CAN_SetValue(h, PCAN_HARD_RESET_STATUS, &on, sizeof(on));
    if (st == PCAN_ERROR_OK) {
        qDebug() <<" CAN RESET";
        st = CAN_Reset(h);   // 硬件级复位，硬件TX缓冲里的 02 04 被清掉
        return st == PCAN_ERROR_OK;
    }

    // 老设备/老驱动不支持该参数，退回原来的 Uninitialize+Initialize
    close_NoLock();
    return init_NoLock(h, baudrate_);
}
