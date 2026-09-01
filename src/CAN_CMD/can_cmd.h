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

// 参数配置
extern const std::vector<uint8_t> SDO_SAVE_DEFAULT_CMD; // 保存默认参数
extern const std::vector<uint8_t> SDO_SAVE_USER_SETTING_CMD; // 保存个人设置
extern const std::vector<uint8_t> SDO_LOAD_DEFAULT_CMD; // 恢复出厂设置

extern const std::vector<uint8_t> SDO_READ_PARAM_TO_TABLE; // 读取参数到表格

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

// 开环模式
extern const std::vector<uint8_t> SDO_OPEN_LOOP_MODE_CMD;
// 闭环模式
extern const std::vector<uint8_t> SDO_CLOSE_LOOP_MODE_CMD;

#define SEND_COB_ID  0x240
// SDO写测试
extern const std::array<uint8_t, 8> SDO_PWM_OPEN_1_VALUE_CMD; //阀1PWM开环
extern const std::array<uint8_t, 8> SDO_PWM_OPEN_2_VALUE_CMD; //阀2PWM开环
extern const std::array<uint8_t, 8> SDO_CUR_OPEN_1_VALUE_CMD; //阀1电流开环
extern const std::array<uint8_t, 8> SDO_CUR_OPEN_2_VALUE_CMD; //阀2电流开环

extern const std::vector<uint8_t> SDO_WRITE_READ_VALUE_CMD;     //读取数值
extern const std::vector<uint8_t> SDO_WRITE_CLOSE_1_CMD;      //关阀1
extern const std::vector<uint8_t> SDO_WRITE_CLOSE_2_CMD;      //关阀2

extern const std::vector<uint8_t> SDO_READ_STAY_1_CMD;      //  读取阀1当前位置
extern const std::vector<uint8_t> SDO_READ_STAY_2_CMD;      //  读取阀2当前位置

#define CUR_STAY_ID 0x5C0
extern const std::vector<uint8_t> SDO_CUR_STAY_1_CMD_HEAD;      //  下位机回复 阀1当前位置
extern const std::vector<uint8_t> SDO_CUR_STAY_2_CMD_HEAD;      //  下位机回复 阀2当前位置

// PID控制
extern const std::array<uint8_t, 8> SDO_WRITE_PID_1_P_CMD; //阀1 P 参数控制
extern const std::array<uint8_t, 8> SDO_WRITE_PID_1_I_CMD; //阀1 I 参数控制
extern const std::array<uint8_t, 8> SDO_WRITE_PID_1_D_CMD; //阀1 D 参数控制

extern const std::array<uint8_t, 8> SDO_WRITE_PID_2_P_CMD; //阀2 P 参数控制
extern const std::array<uint8_t, 8> SDO_WRITE_PID_2_I_CMD; //阀2 I 参数控制
extern const std::array<uint8_t, 8> SDO_WRITE_PID_2_D_CMD; //阀2 D 参数控制

//发送目标值
extern const std::array<uint8_t, 8> SDO_SEND_TARGET_1_VALUE_CMD;
extern const std::array<uint8_t, 8> SDO_SEND_TARGET_2_VALUE_CMD;

//归零与停止
extern const std::vector<uint8_t> SDO_STOP_1_CMD; //阀1归零           0x240   00 00 00 00 11 00 00 11
extern const std::vector<uint8_t> SDO_STOP_2_CMD; //阀2归零           0x240   00 00 00 00 12 00 00 12

// 阶跃模式
extern const std::vector<uint8_t> SDO_STEP_MODE_CMD;
// 斜坡模式
extern const std::vector<uint8_t> SDO_RAMP_MODE_CMD;
// 配置斜坡
#define RPDO2_COB_ID 0x340
extern const std::array<uint8_t, 8> SDO_RPDO2_RAMP_TIME_CMD;
// 伸出激活=500ms
extern const std::vector<uint8_t> SDO_RAMP_EXTEND_TIME_CMD;
extern const std::array<uint8_t, 8> SDO_SET_RAMP_EXTEND_TIME_CMD; //伸出激活： 斜坡时间
// 缩回激活=200ms
extern const std::vector<uint8_t> SDO_RAMP_RETRACT_TIME_CMD;
extern const std::array<uint8_t, 8> SDO_SET_RAMP_RETRACT_TIME_CMD; //缩回激活： 斜坡时间

/// NMT状态
#define NMT_COB_ID  0x000
extern const std::vector<uint8_t> NMT_READ_VALUE_CMD;     //读取位移
extern const std::vector<uint8_t> NMT_CLOSE_READ_CMD;      //关阀

#endif // _CAN_CMD_H_