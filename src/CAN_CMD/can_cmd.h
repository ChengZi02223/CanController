#ifndef _CAN_CMD_H_
#define _CAN_CMD_H_

#include <cstdint>
#include <vector>
#include <array>

#define CAN_EFF_FLAG    0x80000000U  // 扩展帧标记位（最高bit）
#define CAN_RTR_FLAG    0x40000000U  // 远程帧
#define CAN_ERR_FLAG    0x20000000U
#define CAN_EFF_MASK    0x1FFFFFFFU  // 29位ID掩码（清除标志位，取出原始ID）
#define CAN_SFF_MASK    0x000007FFU  // 11位标准ID掩码

/// 切换CANopen
#define CHANGE_TO_CANOPEN_COB_ID  0x18EF8122
#define CHANGE_TO_CANOPEN_RPS_COB_ID  0x18EF2281
extern const std::vector<uint8_t> CHANGE_TO_CANOPEN_CMD;
extern const std::vector<uint8_t> CHANGE_TO_CANOPEN_CMD_RPS;

/// 切换J939
extern const std::vector<uint8_t> CHANGE_TO_J1939_CMD;
extern const std::vector<uint8_t> CHANGE_TO_J1939_CMD_RPS;

/// SDO读写测试
#define SDO_COB_ID  0x640
// SDO读测试
extern const std::vector<uint8_t> SDO_READ_TYPE_CMD;            //设备类型
extern const std::vector<uint8_t> SDO_READ_PROTOCOL_CMD;        //协议
extern const std::vector<uint8_t> SDO_READ_MANUFACTURER_ID_CMD; //厂商ID
extern const std::vector<uint8_t> SDO_READ_STATUS_WORD_CMD;     //状态字
extern const std::vector<uint8_t> SDO_READ_FAULT_WORD_CMD;      //故障字
// 回复信息
extern const std::vector<uint8_t> SDO_READ_TYPE_CMD_RPS;            //设备类型
extern const std::vector<uint8_t> SDO_READ_PROTOCOL_CMD_RPS;        //协议
extern const std::vector<uint8_t> SDO_READ_MANUFACTURER_ID_CMD_RPS; //厂商ID
extern const std::vector<uint8_t> SDO_READ_STATUS_WORD_CMD_RPS;     //状态字
extern const std::vector<uint8_t> SDO_READ_FAULT_WORD_CMD_RPS;      //故障字

// SDO写测试
extern const std::array<uint8_t, 8> SDO_WRITE_OPEN_VALUE_CMD; //开阀
extern const std::vector<uint8_t> SDO_WRITE_READ_VALUE_CMD;     //读取数值
extern const std::vector<uint8_t> SDO_WRITE_CLOSE_CMD;      //关阀

/// NMT状态
#define NMT_COB_ID  0x000
extern const std::vector<uint8_t> NMT_READ_VALUE_CMD;     //读取位移
extern const std::vector<uint8_t> NMT_CLOSE_READ_CMD;      //关阀

#endif // _CAN_CMD_H_