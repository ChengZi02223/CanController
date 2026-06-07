#ifndef CAN_DRIVER_HPP
#define CAN_DRIVER_HPP

#include <cstdint>
#include <string>

// 自定义 CAN 帧结构 (兼容 Linux can_frame 格式)
struct can_frame {
    uint32_t can_id;   // CAN ID (11位或29位，本项目只用11位)
    uint8_t  can_dlc;  // 数据长度 (0..8)
    uint8_t  data[8];  // 数据字节
};

class CanDriver {
public:
    CanDriver();
    ~CanDriver();

    bool init(const std::string& device, uint32_t baudrate);
    void close();
    bool send(const can_frame& frame);
    bool receive(can_frame& frame, int timeout_ms = -1);

private:
    void* handle_;  // 指向 PCAN 句柄的内部类型（实际为TPCANHandle）
    bool isInitialized_;
};

#endif // CAN_DRIVER_HPP