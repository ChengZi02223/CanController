#include "CanDriver.h"
#include "CO_CMD.h"
#include <iostream>
#include <cstring>
#include <vector>
#include "can_cmd.h"
#include "Utils.h"
#include <QDateTime>

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
    // 先关闭已有通道
    close();

    TPCANBaudrate pcanBaud;
    switch (baudrate) {
        case 125000: pcanBaud = PCAN_BAUD_125K; break;
        case 250000: pcanBaud = PCAN_BAUD_250K; break;
        case 500000: pcanBaud = PCAN_BAUD_500K; break;
        case 1000000: pcanBaud = PCAN_BAUD_1M;   break;
        default:      pcanBaud = PCAN_BAUD_250K; break;
    }

    TPCANStatus status = CAN_Initialize(channelHandle, pcanBaud, 0, 0, 0);
    if (status != PCAN_ERROR_OK) {
        char errText[256] = {0};
        CAN_GetErrorText(status, 0x09, errText); // 0x09=英文错误信息
        std::cerr << "CAN初始化失败:" << errText << " 错误码:0x" << std::hex << status << std::endl;
        return false;
    }
    handle_ = reinterpret_cast<void*>(static_cast<uintptr_t>(channelHandle));
    isInitialized_ = true;
    return true;
}

void CanDriver::close()
{
    if (isInitialized_ && handle_) {
        TPCANHandle pcanHandle = static_cast<TPCANHandle>(reinterpret_cast<uintptr_t>(handle_));
        CAN_Uninitialize(pcanHandle);
        handle_ = nullptr;
        isInitialized_ = false;
    }
}

bool CanDriver::SendCmd(const uint32_t cobId, const std::vector<uint8_t>& cmd, int timeout_ms) {
    std::lock_guard<std::recursive_mutex> lock(m_io_mtx_);
    PrintCmd(cobId, cmd);
#ifdef ON_TEST_MODE
    return true;
#else
    can_frame frame{};
    frame.can_id = cobId;
    frame.can_dlc = static_cast<uint8_t>(cmd.size()); // 不要硬写8！使用实际长度
    // 拷贝vector到data数组，最多拷贝8字节（CAN标准最大载荷）
    const size_t copyLen = std::min(cmd.size(), sizeof(frame.data));
    std::memcpy(frame.data, cmd.data(), copyLen);

    if(frame.can_id > 0x7FF){
        frame.can_id |= CAN_EFF_FLAG;
    }

    if(!send(frame)) {
        return false;
    }
#endif
}

bool CanDriver::ExecCmd(const uint32_t cobId, const std::vector<uint8_t> cmd, can_frame& response, int timeout_ms) {
    std::lock_guard<std::recursive_mutex> lock(m_io_mtx_);
    SendCmd(cobId, cmd, timeout_ms);
    receive(response, timeout_ms);
    return true;
}

bool CanDriver::ExecCmd(const uint32_t cobId, const std::vector<uint8_t>& cmd, int timeout_ms) {
    std::lock_guard<std::recursive_mutex> lock(m_io_mtx_);
    return SendCmd(cobId, cmd, timeout_ms);
}

bool CanDriver::ExecCmds(const std::vector<CanCmdItem>& cmdList)
{
    // 整个批量发送全程持有互斥锁，保证多条报文连续输出，不被其他ExecCmd抢占
    std::lock_guard<std::recursive_mutex> lock(m_io_mtx_);

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
        if (!send(frame))
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
    std::lock_guard<std::recursive_mutex> lock(m_io_mtx_);
    if (!isInitialized_ || !handle_) return false;
    if(!CRCCheck(frame)) return false;
    TPCANHandle pcanHandle = static_cast<TPCANHandle>(reinterpret_cast<uintptr_t>(handle_));
    TPCANMsg msg;
    msg.ID      = frame.can_id & CAN_EFF_MASK; // 清除标志位，只保留29位ID
    if(frame.can_id & CAN_EFF_FLAG) {
        msg.MSGTYPE = PCAN_MESSAGE_EXTENDED; // 扩展帧！
    } else {
        msg.MSGTYPE = PCAN_MESSAGE_STANDARD;
    }
    msg.LEN     = frame.can_dlc;
    std::memcpy(msg.DATA, frame.data, 8);
    TPCANStatus status = CAN_Write(pcanHandle, &msg);
    if (status != PCAN_ERROR_OK) {
        std::cerr << "发送失败，错误码: 0x" << std::hex << status << std::endl;
        return false;
    }
    return true;
}

// ====================== 原有接收函数优化：增加错误文本 ======================
bool CanDriver::receive(can_frame& frame, int timeout_ms)
{
    std::lock_guard<std::recursive_mutex> lock(m_io_mtx_);

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
        TPCANStatus status = CAN_Read(pcanHandle, &msg, &ts);
        if (status == PCAN_ERROR_OK)
        {
            // 读到有效报文，填充输出frame
            frame.can_id = msg.ID;
            frame.can_dlc = msg.LEN;
            // 按真实DLC拷贝，剩余字节清零，避免栈垃圾
            memset(frame.data, 0, sizeof(frame.data));
            std::memcpy(frame.data, msg.DATA, msg.LEN);
            return true;
        }
        else if (status != PCAN_ERROR_QRCVEMPTY)
        {
            // 不是队列为空，是真实硬件错误
            char errText[256] = {0};
            CAN_GetErrorText(status, 0x09, errText);
            std::cerr << "接收异常:" << errText << " 错误码:0x" << std::hex << status << std::endl;
            return false;
        }
        // status == PCAN_ERROR_QRCVEMPTY，队列空，小sleep让出CPU
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

// ====================== 可选：CAN FD 初始化/收发（适配新款FD硬件） ======================
bool CanDriver::initFD(TPCANHandle channelHandle, const char* fdBitrateStr)
{
    close();
    TPCANStatus status = CAN_InitializeFD(channelHandle, (TPCANBitrateFD)fdBitrateStr);
    if (status != PCAN_ERROR_OK)
    {
        char errText[256] = {0};
        CAN_GetErrorText(status, 0x09, errText);
        std::cerr << "CAN FD初始化失败:" << errText << std::endl;
        return false;
    }
    handle_ = reinterpret_cast<void*>(static_cast<uintptr_t>(channelHandle));
    isInitialized_ = true;
    return true;
}

bool CanDriver::sendFD(TPCANMsgFD& fdMsg)
{
    if (!isInitialized_) return false;
    TPCANHandle h = static_cast<TPCANHandle>(reinterpret_cast<uintptr_t>(handle_));
    TPCANStatus s = CAN_WriteFD(h, &fdMsg);
    return s == PCAN_ERROR_OK;
}

bool CanDriver::receiveFD(TPCANMsgFD& fdMsg, TPCANTimestampFD* ts)
{
    if (!isInitialized_) return false;
    TPCANHandle h = static_cast<TPCANHandle>(reinterpret_cast<uintptr_t>(handle_));
    TPCANStatus s = CAN_ReadFD(h, &fdMsg, ts);
    return s == PCAN_ERROR_OK;
}