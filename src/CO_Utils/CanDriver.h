#ifndef CAN_DRIVER_HPP
#define CAN_DRIVER_HPP
#include <cstdint>
#include <string>
#include <vector>
#include <windows.h>
#include <PCANBasic.h>
#include <mutex>
#include <atomic>

#define kCmdTimeOut 20

// 自定义 CAN 帧结构 (兼容 Linux can_frame 格式)
struct can_frame {
    uint32_t can_id;   // CAN ID (11位或29位)
    uint8_t  can_dlc;  // 数据长度 (0..8)
    uint8_t  data[8];  // 数据字节
};

// 扫描返回的通道信息（给QT界面下拉框用）
struct CanChannelInfo
{
    TPCANHandle handle;
    std::string channelName;   // "PCAN_USBBUS1"
    std::string hardwareName;  // 硬件型号 "PCAN-USB FD"
    bool isAvailable;          // true=未被占用可连接
    bool isFDSupport;          // 是否支持CAN FD
};

struct CanCmdItem
{
    uint32_t cobId;
    std::vector<uint8_t> cmd;
};

class CanDriver {
public:
    static CanDriver* GetInstance() {
        static CanDriver instance;
        return &instance;
    }

    // 【新增】自动扫描本机所有PCAN通道，返回全部通道列表
    std::vector<CanChannelInfo> scanAllChannels();

    // 初始化：现在直接传入扫描得到的handle，不再传字符串设备名
    bool init(TPCANHandle channelHandle, uint32_t baudrate);
    bool IsInitialized() const { return isInitialized_; }
    void close();

    bool SendCmdWithRetry(uint32_t cobId, const std::vector<uint8_t>& cmd, int maxRetries = 3, int retryIntervalMs = 10);
    bool SendCmd(const uint32_t cobId, const std::vector<uint8_t>& cmd, int timeout_ms);
    bool ExecCmd(const uint32_t cobId, const std::vector<uint8_t> cmd, can_frame& response, int timeout_ms = -1);
    bool ExecCmd(const uint32_t cobId, const std::vector<uint8_t>& cmd, int timeout_ms);
    bool ExecCmds(const std::vector<CanCmdItem>& cmdList);
    // 经典CAN收发（原有逻辑保留）
    bool send(const can_frame& frame);
    bool receive(can_frame& frame, int timeout_ms = -1);

    bool CRCCheck(const can_frame& frame);
    void FlushRxBuffer();
    bool FlushBuffers();
    // 【新增】彻底复位通道：连"已进入硬件缓冲区、正在重发的帧"也一并清掉
    bool HardReset();

private:
    CanDriver();
    ~CanDriver();

    bool SendCmd_NoLock(const uint32_t cobId, const std::vector<uint8_t>& cmd, int timeout_ms);
    void close_NoLock();
    bool init_NoLock(TPCANHandle channelHandle, uint32_t baudrate);
    bool send_NoLock(const can_frame& frame); 
    // std::recursive_mutex m_io_mtx_;
    // std::recursive_mutex m_io_mtx_;

    std::mutex m_tx_mtx_;   //发送锁，优先上锁！！！全局顺序：先tx，后rx
    std::mutex m_rx_mtx_;   //接收锁，后上锁

    void* handle_;
    std::atomic<bool> isInitialized_{false};

    uint32_t baudrate_;
};

struct CanQueueCleanGuard {
    ~CanQueueCleanGuard() { CanDriver::GetInstance()->FlushRxBuffer(); }
};

#endif // CAN_DRIVER_HPP