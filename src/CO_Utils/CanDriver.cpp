#include "CanDriver.h"
#include <iostream>
#include <cstring>

static bool GetPcanHandle(const std::string& device, TPCANHandle& outHandle)
{
    auto it = StrToDevMap.find(device);
    if (it != StrToDevMap.end())
    {
        outHandle = it->second;
        return true;
    }
    return false;
}

CanDriver::CanDriver()
    : handle_(nullptr), isInitialized_(false)
{
}

CanDriver::~CanDriver()
{
    close();
}

bool CanDriver::init(const std::string& device, uint32_t baudrate)
{
    TPCANHandle pcanHandle;
    if (!GetPcanHandle(device, pcanHandle)) {
        std::cerr << "不支持设备: " << device << std::endl;
        return false;
    }

    TPCANBaudrate pcanBaud;
    switch (baudrate) {
        case 125000: pcanBaud = PCAN_BAUD_125K; break;
        case 250000: pcanBaud = PCAN_BAUD_250K; break;
        case 500000: pcanBaud = PCAN_BAUD_500K; break;
        case 1000000: pcanBaud = PCAN_BAUD_1M;   break;
        default:      pcanBaud = PCAN_BAUD_250K; break;
    }

    TPCANStatus status = CAN_Initialize(pcanHandle, pcanBaud, 0, 0, 0);
    if (status != PCAN_ERROR_OK) {
        std::cerr << "CAN 初始化失败，错误码: 0x" << std::hex << status << std::endl;
        return false;
    }

    handle_ = reinterpret_cast<void*>(static_cast<uintptr_t>(pcanHandle));
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

bool CanDriver::send(const can_frame& frame)
{
    if (!isInitialized_ || !handle_) return false;

    TPCANHandle pcanHandle = static_cast<TPCANHandle>(reinterpret_cast<uintptr_t>(handle_));
    TPCANMsg msg;
    msg.ID      = frame.can_id & 0x7FF;
    msg.MSGTYPE = PCAN_MESSAGE_STANDARD;
    msg.LEN     = frame.can_dlc;
    std::memcpy(msg.DATA, frame.data, 8);

    TPCANStatus status = CAN_Write(pcanHandle, &msg);
    if (status != PCAN_ERROR_OK) {
        std::cerr << "发送失败，错误码: 0x" << std::hex << status << std::endl;
        return false;
    }
    return true;
}

bool CanDriver::receive(can_frame& frame, int timeout_ms)
{
    if (!isInitialized_ || !handle_) return false;

    TPCANHandle pcanHandle = static_cast<TPCANHandle>(reinterpret_cast<uintptr_t>(handle_));
    TPCANMsg msg;
    TPCANTimestamp ts;

    TPCANStatus status = CAN_Read(pcanHandle, &msg, &ts);
    if (status == PCAN_ERROR_QRCVEMPTY) {
        if (timeout_ms > 0) Sleep(timeout_ms);
        return false;
    } else if (status != PCAN_ERROR_OK) {
        std::cerr << "接收异常，错误码: 0x" << std::hex << status << std::endl;
        return false;
    }

    frame.can_id  = msg.ID;
    frame.can_dlc = msg.LEN;
    std::memcpy(frame.data, msg.DATA, 8);
    return true;
}