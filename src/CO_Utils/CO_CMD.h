/**
 * @file command.h
 * @brief CANopen 上位机（主站）发送给下位机（从站）的所有 CAN 命令定义
 * 
 * 基于海卓力克 CAN 总线多路阀 CANopen 控制协议 V3（修订时间 2026.5.27）
 * 包含：NMT 网络管理命令、SDO 服务数据对象命令、LSS 层设置服务命令、
 *       SYNC 同步报文、RPDO 接收过程数据对象命令。
 * 
 * 说明：
 * - 所有命令的 COB-ID 均需结合节点地址 (NodeID, 1~127) 计算。
 * - PDO 数据内容由映射定义决定，本文件提供 RPDO 的 COB-ID 宏和常用数据内容构造建议。
 * - 控制字 0x6040 的标准值宏一并给出，便于快速使能/复位设备。
 */

#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * 1. NMT 网络管理命令 (COB-ID = 0x000)
 *    数据长度 2 字节: [NMT命令, NodeID] (NodeID=0 表示广播)
 *============================================================================*/
#define NMT_COB_ID                      0x000u

/* NMT 命令码 (Command Specifier) */
#define NMT_CMD_START_NODE              0x01u   /* 启动节点 -> Operational 状态 */
#define NMT_CMD_STOP_NODE               0x02u   /* 停止节点 -> Stopped 状态 */
#define NMT_CMD_ENTER_PREOPERATIONAL    0x80u   /* 进入预操作状态 */
#define NMT_CMD_RESET_NODE              0x81u   /* 复位节点 (全复位) */
#define NMT_CMD_RESET_COMMUNICATION     0x82u   /* 复位通信 (仅协议栈) */
#define NMT_CMD_CLEAR_FAULT             0x83u   /* 清除故障 */

/* 辅助函数：构建 NMT 命令帧数据 (返回 2 字节数据) */
static inline uint16_t nmt_command_payload(uint8_t cmd, uint8_t node_id) {
    return ((uint16_t)cmd << 8) | (node_id & 0x7F);
}

/*=============================================================================
 * 2. SDO 服务数据对象命令 (主站 -> 从站，COB-ID = 0x600 + NodeID)
 *    快速下载/上传，分段传输等
 *============================================================================*/
#define SDO_BASE_COB_ID(node)           (0x600u + (node))

/* 快速下载 (写) 命令码 */
#define SDO_CMD_DOWNLOAD_1_BYTE         0x2Fu   /* 写入 1 字节数据 */
#define SDO_CMD_DOWNLOAD_2_BYTE         0x2Bu   /* 写入 2 字节数据 */
#define SDO_CMD_DOWNLOAD_3_BYTE         0x27u   /* 写入 3 字节数据 */
#define SDO_CMD_DOWNLOAD_4_BYTE         0x23u   /* 写入 4 字节数据 */

/* 快速上传 (读) 命令码 */
#define SDO_CMD_UPLOAD_REQUEST          0x40u   /* 请求读取对象字典 */

/* 分段下载命令码 */
#define SDO_CMD_SEG_DOWNLOAD_INIT       0x00u   /* 分段下载初始化 (带总长度) */
#define SDO_CMD_SEG_DOWNLOAD_DATA       0x00u   /* 分段下载数据段 (奇数次，toggle=0) */
#define SDO_CMD_SEG_DOWNLOAD_DATA_TGL   0x10u   /* 分段下载数据段 (偶数次，toggle=1) */

/* 分段上传命令码 (主站请求) */
#define SDO_CMD_SEG_UPLOAD_REQUEST      0x40u   /* 上传初始化请求 */
#define SDO_CMD_SEG_UPLOAD_NEXT         0x60u   /* 请求下一个分段 (toggle=0) */
#define SDO_CMD_SEG_UPLOAD_NEXT_TGL     0x70u   /* 请求下一个分段 (toggle=1) */

/* 中止传输 */
#define SDO_CMD_ABORT                   0x80u   /* SDO 中止传输 (从站返回，主站也可发送？通常从站发送) */

/*=============================================================================
 * 3. LSS 层设置服务命令 (主站 -> 从站，COB-ID = 0x7E5)
 *    所有帧均为 8 字节数据，定义参见 CiA 305 标准
 *============================================================================*/
#define LSS_MASTER_COB_ID               0x7E5u
#define LSS_SLAVE_COB_ID                0x7E4u   /* 从站响应，非命令，仅用于参考 */

/* LSS 命令码 (Byte0) */
#define LSS_CMD_CONFIGURE_NODE_ID       0x11u   /* 配置节点 ID (后跟新 NodeID) */
#define LSS_CMD_CONFIGURE_BITRATE       0x13u   /* 配置波特率 (后跟索引表值) */
#define LSS_CMD_ACTIVATE_BITRATE        0x15u   /* 激活波特率 (后跟延迟时间) */
#define LSS_CMD_STORE_CONFIG            0x17u   /* 保存配置到非易失存储器 */
#define LSS_CMD_SWITCH_GLOBAL           0x04u   /* 全局状态切换 (0=等待,1=配置) */
#define LSS_CMD_SWITCH_SELECT_VENDOR    0x40u   /* 按厂商 ID 选择从站 */
#define LSS_CMD_SWITCH_SELECT_PRODUCT   0x41u   /* 按产品代码选择 */
#define LSS_CMD_SWITCH_SELECT_REVISION  0x42u   /* 按修订号选择 */
#define LSS_CMD_SWITCH_SELECT_SERIAL    0x43u   /* 按序列号选择 */
#define LSS_CMD_SWITCH_SELECT_RESPONSE  0x44u   /* 选择切换响应 (从站->主站) */
#define LSS_CMD_INQUIRE_VENDOR          0x5Au   /* 查询厂商 ID */
#define LSS_CMD_INQUIRE_PRODUCT         0x5Bu   /* 查询产品码 */
#define LSS_CMD_INQUIRE_REVISION        0x5Cu   /* 查询修订号 */
#define LSS_CMD_INQUIRE_SERIAL          0x5Du   /* 查询序列号 */
#define LSS_CMD_INQUIRE_NODE_ID         0x5Eu   /* 查询当前节点 ID */

/* 波特率索引值 (用于 LSS_CMD_CONFIGURE_BITRATE 的 Byte1) */
#define LSS_BAUDRATE_1000K              0x00u
#define LSS_BAUDRATE_800K               0x01u
#define LSS_BAUDRATE_500K               0x02u
#define LSS_BAUDRATE_250K               0x03u
#define LSS_BAUDRATE_125K               0x04u
#define LSS_BAUDRATE_50K                0x06u   /* 或 0x07，根据文档使用 0x06 */
#define LSS_BAUDRATE_10K                0x08u

/*=============================================================================
 * 4. SYNC 同步报文 (COB-ID = 0x80，无数据)
 *============================================================================*/
#define SYNC_COB_ID                     0x80u

/*=============================================================================
 * 5. RPDO 接收过程数据对象命令 (主站 -> 从站)
 *    根据协议映射定义，RPDO1/2/3 的 COB-ID 基址如下：
 *        RPDO1: 0x200 + NodeID
 *        RPDO2: 0x300 + NodeID
 *        RPDO3: 0x400 + NodeID
 *    数据内容 (最多 8 字节) 需按映射顺序打包，常用映射：
 *        RPDO1: [0x6040 控制字(2B), 0x6300 位置设定点(2B), 0x6043 细分模式(1B)]
 *        RPDO2: [0x6380 压力备用(2B), 0x6042 运行模式(1B), 0x6302 传感器通道(1B)]
 *        RPDO3: [0x6361 抖动幅度(2B), 0x6362 抖动频率(2B), 0x6330 斜坡类型(1B)]
 *============================================================================*/
#define RPDO1_COB_ID(node)              (0x200u + (node))
#define RPDO2_COB_ID(node)              (0x300u + (node))
#define RPDO3_COB_ID(node)              (0x400u + (node))

/* 常用 RPDO 数据长度 (字节) */
#define RPDO1_DLC                       5u      /* 2+2+1 = 5 */
#define RPDO2_DLC                       4u      /* 2+1+1 = 4 */
#define RPDO3_DLC                       5u      /* 2+2+1 = 5 */

/*=============================================================================
 * 6. 控制字 0x6040 常用值 (通过 RPDO1 发送)
 *    这些不是独立的 CAN 命令，而是 PDO 数据内容，为方便使用列出。
 *============================================================================*/
/* 控制字位定义 */
#define CTRL_BIT_SWITCH_ON              (1u << 0)   /* 上电使能 */
#define CTRL_BIT_ENABLE_VOLTAGE         (1u << 1)   /* 电压使能 */
#define CTRL_BIT_QUICK_STOP             (1u << 2)   /* 快速停止 (0 激活快停，1 解除) */
#define CTRL_BIT_ENABLE_OPERATION       (1u << 3)   /* 运行使能 */
#define CTRL_BIT_NEW_SETPOINT           (1u << 4)   /* 新设定点触发 */
#define CTRL_BIT_CHANGE_IMMEDIATE       (1u << 5)   /* 立即变更设定值 */
#define CTRL_BIT_ABS_REL                (1u << 6)   /* 绝对/相对位置 */
#define CTRL_BIT_FAULT_RESET            (1u << 7)   /* 故障复位上升沿 */
#define CTRL_BIT_HALT                   (1u << 8)   /* 受控暂停 */

/* 标准状态转换对应的控制字值 */
#define CTRL_SHUTDOWN                   0x0006u     /* Switch on disabled -> Ready to switch on */
#define CTRL_SWITCH_ON                  0x0007u     /* Ready to switch on -> Switched on */
#define CTRL_ENABLE_OPERATION           0x000Fu     /* Switched on -> Operation enabled */
#define CTRL_DISABLE_OPERATION          0x0007u     /* Operation enabled -> Switched on */
#define CTRL_QUICK_STOP_ACTIVE          0x0002u     /* 触发快速停止 */
#define CTRL_FAULT_RESET_CMD            0x0080u     /* 故障复位 (上升沿，需后续清零) */

/*=============================================================================
 * 7. 辅助宏：紧急报文 COB-ID (从站发送，主站仅需监听，非命令)
 *    为完整性给出定义
 *============================================================================*/
#define EMCY_COB_ID(node)               (0x80u + (node))

/*=============================================================================
 * 8. 心跳相关 (主站作为消费者，不发送心跳命令，仅配置)
 *    生产者心跳周期 0x1017 由主站通过 SDO 配置，无专门命令帧
 *============================================================================*/

#ifdef __cplusplus
}
#endif

#endif /* __COMMAND_H__ */