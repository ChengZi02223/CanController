#ifndef _UTILS_H
#define _UTILS_H

#include <vector>
#include <cstdint>
#include <QDebug>
#include <iostream>
#include <cstring>
#include "CanDriver.h"
#define kSleepTimeOut 300
#define kReadTPDOTimeOut 200
#define kFaMaxFlow 80 // L/Min
#define kThreadWaitTime 200

#define kCloseCycleWaitTime 5 //s
#define ON_TEST_MODE

static const std::vector<int> Calibrat_list = {100, 50, 25, 10, 0, 0, 0, 10, 25, 50, 100};
static const std::vector<double> flow_table = {1.0, 0.5, 0.25, 0.1, 0, 0, 0, 0.1, 0.25, 0.5, 1.0};
// static const std::vector<int> flow_yugu_values = {154, 132, 115, 110, 99, 0, 68, 62, 53, 39, 23};  //预估值 10月13日前
static const std::vector<int> flow_yugu_values = {159, 132, 115, 111, 99, 0, 65, 59, 49, 34, 7};  //预估值
static const std::vector<int> flow_yugu_value_subidxs = {0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E};  //预估值
static const std::vector<int> flow_curr_value_subidxs = {0x12, 0x13, 0x14, 0x15, 0x16, 0xFF, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E};  //预估值

// #define CAN_MGR_PUSH_CMD(id, cmd)  CanDriver::GetInstance()->SendCmd(id, cmd, kCmdTimeOut);
// #define CAN_EXEC_CMD_WITH_RETRY(id, cmd)  CanDriver::GetInstance()->SendCmdWithRetry(id, cmd, 3, 10);

// #define CAN_CLEAR_BUFF CanDriver::GetInstance()->FlushBuffers(); \
//                        CanDriver::GetInstance()->FlushRxBuffer();  

// #define CAN_QUEUE_CLEAN_GUARD CanQueueCleanGuard guard;

inline int32_t GetTargetFlow(int i) {
    if(i >= 0 || i < flow_table.size()) {
        return flow_table[i] * kFaMaxFlow;
    }
    return 0;
} 

inline int32_t GetYuGuValueSubIdx(int i) {
    if(i >= 0 || i < flow_yugu_value_subidxs.size()) {
        return flow_yugu_value_subidxs[i];
    }
    return 0xFF;
} 

inline int32_t GetCurrValueSubIdx(int i) {
    if(i >= 0 || i < flow_curr_value_subidxs.size()) {
        return flow_curr_value_subidxs[i];
    }
    return 0xFF;
} 

inline int GetCalibratValue(int i) {
    if(i >= 0 || i < Calibrat_list.size()) {
        return Calibrat_list[i];
    }
    return 0;
}

inline int GetCalibratYuGuValue(int i) {
    if(i >= 0 || i < flow_yugu_values.size()) {
        return flow_yugu_values[i];
    }
    return 0;
}

inline double CalcDisplacement(double y, int i)
{
    // tableA i<6   pair:{y输入, x输出}
    static const std::vector<std::pair<int, int>> tableA = {
        {99,   0},
        {110,  10},
        {132,  50},
        {150,  90},
        {154, 100}
    };
    // tableB i>=6  pair:{y输入, x输出}
    static const std::vector<std::pair<int, int>> tableB = {
        {68,   0},
        {62,  10},
        {39,  50},
        {22,  90},
        {23, 100}
    };

    const std::vector<std::pair<int, int>>* table;
    if (i < 6)
        table = &tableA;
    else
        table = &tableB;

    // -------- 限幅 --------
    if(i < 6)
    {
        // tableA y升序
        if(y <= table->at(0).first)
            return static_cast<double>(table->at(0).second);
        if(y >= table->back().first)
            return static_cast<double>(table->back().second);
    }
    else
    {
        // tableB：大于第一个点y=68 →输出x=0；小于y=22输出x=90；大于23输出x=100
        if(y >= table->at(0).first)
            return static_cast<double>(table->at(0).second);
        if(y <= table->at(3).first) // y <=22
            return static_cast<double>(table->at(3).second);
        // 22~23区间
        if(y >= table->at(4).first)
            return static_cast<double>(table->at(4).second);
    }

    // 遍历查找所在区间
    for(int idx = 0; idx < static_cast<int>(table->size()) - 1; ++idx)
    {
        int y0 = table->at(idx).first;
        int x0 = table->at(idx).second;
        int y1 = table->at(idx+1).first;
        int x1 = table->at(idx+1).second;

        bool inRange;
        if(y0 < y1)
        {
            inRange = (y >= y0 && y <= y1);
        }
        else
        {
            inRange = (y <= y0 && y >= y1);
        }

        if(inRange)
        {
            double k = static_cast<double>(x1 - x0) / (y1 - y0);
            double x = x0 + k * (y - y0);
            return x;
        }
    }

    return static_cast<double>(table->at(0).second);
}

// 提取响应中指定字节数据
inline uint16_t ExtractFromVectorData(const std::vector<uint8_t>& data, int byte1, int byte2) {
    // 小端序：前字节为低字节，后字节为高字节
    return static_cast<uint16_t>(data[byte1]) | 
                     (static_cast<uint16_t>(data[byte2]) << 8);
}

inline uint16_t ExtractFromDataList(const uint8_t data[8], int byte1, int byte2) {
    // 小端序：前字节为低字节，后字节为高字节
    return static_cast<uint16_t>(data[byte1]) | 
                     (static_cast<uint16_t>(data[byte2]) << 8);
}

static std::vector<uint8_t> SetTargetCMDValue(const std::array<uint8_t, 8> command, int val, int factor = 10) {
    uint16_t value = static_cast<uint16_t>(val * factor); // 100 → 1000

    // 2. 复制模板生成待发送指令
    std::vector<uint8_t> cmd(command.begin(), command.end());

    // 3. 【小端模式】填充第3、4字节（下标2、3）
    cmd[2] = static_cast<uint8_t>(value & 0xFF);        // 低字节 0xE8
    cmd[3] = static_cast<uint8_t>((value >> 8) & 0xFF); // 高字节 0x03

    uint8_t sum = 0;
    for(int i = 0; i < 7; ++i) {
        sum += cmd[i];
    }
    cmd[7] = sum & 0xFF; // 取低8位
    return cmd;
}

static std::vector<uint8_t> SetPIDCMDValue(const std::array<uint8_t, 8> command, double val) {
    uint16_t value = static_cast<uint16_t>(val * 1000); 

    // 2. 复制模板生成待发送指令
    std::vector<uint8_t> cmd(command.begin(), command.end());

    // 3. 【小端模式】填充第5、6字节（下标4、5）
    cmd[4] = static_cast<uint8_t>(value & 0xFF);        // 低字节 0xE8
    cmd[5] = static_cast<uint8_t>((value >> 8) & 0xFF); // 高字节 0x03

    return cmd;
}

// RPDO2斜坡配置     0x340   01 FA FA FF 19 19 19 19  RPDO2 的斜坡时间编码为 20ms/bit ，即 0x19=25，25×20=500ms
static std::vector<uint8_t> RampRadioAndTimeCMDConfig(const std::array<uint8_t, 8> command, double radio, double time) {
    uint8_t radio_value = static_cast<uint8_t>(radio);
    uint16_t time_value = static_cast<uint16_t>(time); 

    std::vector<uint8_t> cmd(command.begin(), command.end());

    cmd[1] = radio_value;
    cmd[2] = radio_value;

    uint8_t value_8 = static_cast<uint8_t>(time_value);

    uint8_t low_byte  = static_cast<uint8_t>(time_value & 0xFF);      // 低字节 0x88
    uint8_t high_byte = static_cast<uint8_t>((time_value >> 8) & 0xFF); // 高字节 0x13

    cmd[4] = low_byte;
    cmd[5] = high_byte;
    cmd[6] = low_byte;
    cmd[7] = high_byte;

    return cmd;
}

static std::vector<uint8_t> RampTimeCMDConfig(const std::array<uint8_t, 8> command, double time) {
    uint16_t value = static_cast<uint16_t>(time); 

    std::vector<uint8_t> cmd(command.begin(), command.end());
    
    uint8_t value_8 = static_cast<uint8_t>(value);

    uint8_t low_byte  = static_cast<uint8_t>(value & 0xFF);      // 低字节 0x88
    uint8_t high_byte = static_cast<uint8_t>((value >> 8) & 0xFF); // 高字节 0x13

    cmd[4] = low_byte;
    cmd[5] = high_byte;
    cmd[6] = low_byte;
    cmd[7] = high_byte;

    return cmd;
}


//十六进制字符串转 uint16_t
static uint16_t QStringToUint16Hex(const QString& strVal) {
    bool ok = false;
    // 使用基数 0 让 Qt 自动根据前缀识别进制（0x 为十六进制，0 为八进制，否则十进制）
    unsigned int val = strVal.toUInt(&ok, 0);
    if (!ok || val > 0xFFFFU)
        return 0x0000;
    return static_cast<uint16_t>(val);
}

//十六进制字符串转 uint8_t
static uint8_t QStringToUint8Hex(const QString& strVal) {
    bool ok = false;
    unsigned int val = strVal.toUInt(&ok, 0);
    if (!ok || val > 0xFFU)
        return 0x00;
    return static_cast<uint8_t>(val);
}

// 十进制字符串 -> uint8_t（范围 0~255）
static uint8_t QStringToUint8Dec(const QString& strVal) {
    bool ok = false;
    unsigned int val = strVal.toUInt(&ok, 10);  // 强制十进制
    if (!ok || val > 0xFFU) {
        qWarning() << "QStringToUint8Dec: 转换失败或超出范围，输入:" << strVal;
        return 0;
    }
    return static_cast<uint8_t>(val);
}

// 十进制字符串 -> uint16_t（范围 0~65535）
static uint16_t QStringToUint16Dec(const QString& strVal) {
    bool ok = false;
    unsigned int val = strVal.toUInt(&ok, 10);
    if (!ok || val > 0xFFFFU) {
        qWarning() << "QStringToUint16Dec: 转换失败或超出范围，输入:" << strVal;
        return 0;
    }
    return static_cast<uint16_t>(val);
}

// 十进制字符串 -> uint32_t（范围 0~4294967295）
static uint32_t QStringToUint32Dec(const QString& strVal) {
    bool ok = false;
    uint32_t val = strVal.toUInt(&ok, 10);  // 注意 Qt 的 toUInt 返回 uint，最大 2^32-1，OK
    if (!ok) {
        qWarning() << "QStringToUint32Dec: 转换失败，输入:" << strVal;
        return 0;
    }
    // toUInt 本身已保证范围，无需额外检查（但若需严格，可追加）
    return val;
}

// static void Fill10ValueToCmd(int indexValue, std::vector<uint8_t>& command, int start_index) {
//     //16位占用2字节，需要 start_index +1 < command.size()
//     if(start_index < 0 || (start_index + 1) >= static_cast<int>(command.size())) {
//         qWarning() << "Invalid start_index or span for Fill10ValueToCmd";
//         return;
//     }
    
//     command[start_index]     = static_cast<uint8_t>(indexValue);         // 低字节
//     command[start_index + 1] = static_cast<uint8_t>((indexValue >> 8) & 0xFF); // 高字节
// }

static void Fill16ValueToCmd(uint16_t indexValue, std::vector<uint8_t>& command, int start_index) {
    //16位占用2字节，需要 start_index +1 < command.size()
    if(start_index < 0 || (start_index + 1) >= static_cast<int>(command.size())) {
        qWarning() << "Invalid start_index or span for Fill16ValueToCmd";
        return;
    }
    // 小端：低字节存start_index，高字节存start_index+1
    command[start_index]     = static_cast<uint8_t>(indexValue & 0xFF);         // 低字节
    command[start_index + 1] = static_cast<uint8_t>((indexValue >> 8) & 0xFF); // 高字节
}


static void Fill32ValueToCmd(uint32_t value, std::vector<uint8_t>& command, int start_index)
{
    //32位占4字节，start_index+3不能超出vector有效范围
    if(start_index < 0 || (start_index + 3) >= static_cast<int>(command.size()))
    {
        qWarning() << "Invalid start_index for Fill32ValueToCmd";
        return;
    }
    // 小端模式：低字节放在起始下标，依次往上
    command[start_index]     = static_cast<uint8_t>(value & 0xFF);         // byte0 低字节
    command[start_index + 1] = static_cast<uint8_t>((value >> 8)  & 0xFF); // byte1
    command[start_index + 2] = static_cast<uint8_t>((value >> 16) & 0xFF); // byte2
    command[start_index + 3] = static_cast<uint8_t>((value >> 24) & 0xFF); // byte3 高字节
}


static void PrintCmd(const uint32_t cobId, const std::vector<uint8_t> &cmd, QString prefix = "cmd") {
    // std::cout << prefix << "(" << QString("0x%1").arg(cobId, 2, 16, QChar('0')).toUpper().toStdString() << "): ";
    // for (auto byte : cmd) {
    //     std::cout << QString("%1").arg(byte, 2, 16, QChar('0')).toUpper().toStdString() << " ";
    // }
    // std::cout << std::endl;
#ifdef ON_TEST_MODE
    QString hexPayload;
    for (auto byte : cmd)
    {
        hexPayload += QString("%1 ").arg(byte,2,16,QLatin1Char('0')).toUpper();
    }
    qDebug().noquote() << prefix << "(0x" <<QString("%1):").arg(cobId,4,16,QLatin1Char('0')).toUpper() << hexPayload;
#endif
}

static void PrintCanFrame(const can_frame &frame, QString prefix = "can_frame")
{
    QString hexPayload;
    for (int i = 0; i < frame.can_dlc; ++i)
    {
        hexPayload += QString("%1 ").arg(frame.data[i], 2, 16, QLatin1Char('0')).toUpper();
    }
    qDebug().noquote() << prefix << "(0x"
                       << QString("%1):").arg(frame.can_id,4,16,QLatin1Char('0')).toUpper()
                       << " dlc:" << frame.can_dlc
                       << hexPayload;
}

inline can_frame MakeCanFrame(const uint32_t cobId, const std::vector<uint8_t> &cmd)
{
    can_frame frame{}; // 全部初始化为0
    frame.can_id = cobId;

    // 最多拷贝8字节，CAN2.0帧最大8字节
    const size_t copyLen = std::min(cmd.size(), (size_t)8);
    std::memcpy(frame.data, cmd.data(), copyLen);

    frame.can_dlc = static_cast<uint8_t>(copyLen);
    return frame;
}
static bool CheckAnswerHead(const uint8_t data[8], const std::vector<uint8_t> &head) {
    if(head.size() !=8U) return false;
    return std::equal(head.begin(), head.end(), data);
}

static bool CheckResponseData(const std::vector<uint8_t> &data, const std::vector<uint8_t> &answer) {
    if(data.size() != answer.size())
        return false;
    return std::equal(data.begin(), data.end(), answer.begin());
}

struct TestFrame {
    uint32_t cobId;
    uint8_t  data[8];
};

static std::vector<TestFrame> kBatchReadTestFrames = {
    //1)应答帧
    {0x5C0, {0x60, 0x10, 0x10, 0x03, 0x00, 0x00, 0x00, 0x00}},

    //========业务数据帧 共61帧，全部来自你的OD日志[]内8字节========
    {0x4C0, {0x03,0x0E,0x10,0x00,0x40,0x07,0x00,0x00}},  //UP01 NUM 0x100E:00 =1856
    {0x4C0, {0x01,0x00,0x20,0x01,0x01,0x00,0x00,0x00}},  //UP02 NUM 0x2000:01 =1
    {0x4C0, {0x01,0x00,0x20,0x02,0x01,0x00,0x00,0x00}},  //UP03 NUM 0x2000:02 =1
    {0x4C0, {0x01,0x03,0x20,0x00,0x0F,0x00,0x00,0x00}},  //UP04 NUM 0x2003:00 =15
    {0x4C0, {0x02,0x03,0x20,0x01,0x9A,0x00,0x00,0x00}},  //UP05 NUM 0x2003:01 =154
    {0x4C0, {0x02,0x03,0x20,0x02,0x96,0x00,0x00,0x00}},  //UP06 NUM 0x2003:02 =150
    {0x4C0, {0x02,0x03,0x20,0x03,0x84,0x00,0x00,0x00}},  //UP07 NUM 0x2003:03 =132
    {0x4C0, {0x02,0x03,0x20,0x04,0x73,0x00,0x00,0x00}},  //UP08 NUM 0x2003:04 =115
    {0x4C0, {0x02,0x03,0x20,0x05,0x6E,0x00,0x00,0x00}},  //UP09 NUM 0x2003:05 =110
    {0x4C0, {0x02,0x03,0x20,0x06,0x63,0x00,0x00,0x00}},  //UP10 NUM 0x2003:06 =99
    {0x4C0, {0x02,0x03,0x20,0x07,0x59,0x00,0x00,0x00}},  //UP11 NUM 0x2003:07 =89
    {0x4C0, {0x02,0x03,0x20,0x08,0x54,0x00,0x00,0x00}},  //UP12 NUM 0x2003:08 =84
    {0x4C0, {0x01,0x03,0x20,0x09,0x05,0x00,0x00,0x00}},  //UP13 NUM 0x2003:09 =5
    {0x4C0, {0x02,0x03,0x20,0x0A,0x44,0x00,0x00,0x00}},  //UP14 NUM 0x2003:10 =68
    {0x4C0, {0x02,0x03,0x20,0x0B,0x3E,0x00,0x00,0x00}},  //UP15 NUM 0x2003:11 =62
    {0x4C0, {0x02,0x03,0x20,0x0C,0x35,0x00,0x00,0x00}},  //UP16 NUM 0x2003:12 =53
    {0x4C0, {0x02,0x03,0x20,0x0D,0x27,0x00,0x00,0x00}},  //UP17 NUM 0x2003:13 =39
    {0x4C0, {0x02,0x03,0x20,0x0E,0x19,0x00,0x00,0x00}},  //UP18 NUM 0x2003:14 =25
    {0x4C0, {0x02,0x03,0x20,0x0F,0x17,0x00,0x00,0x00}},  //UP19 NUM 0x2003:15 =23
    {0x4C0, {0x02,0x03,0x20,0x11,0x00,0x00,0x00,0x00}},  //UP20 NUM 0x2003:17 =0
    {0x4C0, {0x02,0x03,0x20,0x12,0x00,0x00,0x00,0x00}},  //UP21 NUM 0x2003:18 =0
    {0x4C0, {0x02,0x03,0x20,0x13,0x00,0x00,0x00,0x00}},  //UP22 NUM 0x2003:19 =0
    {0x4C0, {0x02,0x03,0x20,0x14,0x00,0x00,0x00,0x00}},  //UP23 NUM 0x2003:20 =0
    {0x4C0, {0x02,0x03,0x20,0x15,0x00,0x00,0x00,0x00}},  //UP24 NUM 0x2003:21 =0
    {0x4C0, {0x02,0x03,0x20,0x16,0x00,0x00,0x00,0x00}},  //UP25 NUM 0x2003:22 =0
    {0x4C0, {0x02,0x03,0x20,0x1A,0x00,0x00,0x00,0x00}},  //UP26 NUM 0x2003:26 =0
    {0x4C0, {0x02,0x03,0x20,0x1B,0x00,0x00,0x00,0x00}},  //UP27 NUM 0x2003:27 =0
    {0x4C0, {0x02,0x03,0x20,0x1C,0x00,0x00,0x00,0x00}},  //UP28 NUM 0x2003:28 =0
    {0x4C0, {0x02,0x03,0x20,0x1D,0x00,0x00,0x00,0x00}},  //UP29 NUM 0x2003:29 =0
    {0x4C0, {0x02,0x03,0x20,0x1E,0x00,0x00,0x00,0x00}},  //UP30 NUM 0x2003:30 =0
    {0x4C0, {0x02,0x03,0x20,0x1F,0x00,0x00,0x00,0x00}},  //UP31 NUM 0x2003:31 =0
    {0x4C0, {0x01,0x0A,0x20,0x00,0x50,0x00,0x00,0x00}},  //UP32 NUM 0x200A:00 =80
    {0x4C0, {0x01,0x0B,0x20,0x00,0x50,0x00,0x00,0x00}},  //UP33 NUM 0x200B:00 =80
    {0x4C0, {0x02,0x20,0x20,0x01,0x00,0x00,0x00,0x00}},  //UP34 NUM 0x2020:01 =0
    {0x4C0, {0x02,0x20,0x20,0x02,0xE8,0x03,0x00,0x00}},  //UP35 NUM 0x2020:02 =1000
    {0x4C0, {0x02,0x20,0x20,0x03,0x64,0x00,0x00,0x00}},  //UP36 NUM 0x2020:03 =100
    {0x4C0, {0x02,0x20,0x20,0x04,0x0A,0x00,0x00,0x00}},  //UP37 NUM 0x2020:04 =10
    {0x4C0, {0x02,0x20,0x20,0x05,0x00,0x00,0x00,0x00}},  //UP38 NUM 0x2020:05 =0
    {0x4C0, {0x02,0x20,0x20,0x06,0xE8,0x03,0x00,0x00}},  //UP39 NUM 0x2020:06 =1000
    {0x4C0, {0x02,0x20,0x20,0x07,0x64,0x00,0x00,0x00}},  //UP40 NUM 0x2020:07 =100
    {0x4C0, {0x02,0x20,0x20,0x08,0x0A,0x00,0x00,0x00}},  //UP41 NUM 0x2020:08 =10
    {0x4C0, {0x02,0x20,0x20,0x09,0xE8,0x03,0x00,0x00}},  //UP42 NUM 0x2020:09 =1000
    {0x4C0, {0x01,0x20,0x20,0x0A,0x01,0x00,0x00,0x00}},  //UP43 NUM 0x2020:10 =1
    {0x4C0, {0x04,0x52,0x60,0x00,0x00,0x32,0x30,0x32}},  //UP44 STR 0x6052:00 seg0
    {0x4C0, {0x04,0x52,0x60,0x00,0x01,0x36,0x30,0x39}},  //UP45 STR 0x6052:00 seg1
    {0x4C0, {0x04,0x52,0x60,0x00,0x02,0x30,0x31,0x30}},  //UP46 STR 0x6052:00 seg2
    {0x4C0, {0x04,0x52,0x60,0x00,0x83,0x30,0x31,0x00}},  //UP47 STR 0x6052:00 seg3
    {0x4C0, {0x04,0x57,0x60,0x00,0x00,0x48,0x61,0x69}},  //UP48 STR 0x6057:00 seg0
    {0x4C0, {0x04,0x57,0x60,0x00,0x01,0x5A,0x68,0x75}},  //UP49 STR 0x6057:00 seg1
    {0x4C0, {0x04,0x57,0x60,0x00,0x02,0x6F,0x4C,0x69}},  //UP50 STR 0x6057:00 seg2
    {0x4C0, {0x04,0x57,0x60,0x00,0x83,0x6B,0x65,0x00}},  //UP51 STR 0x6057:00 seg3
    {0x4C0, {0x01,0x58,0x60,0x00,0x00,0x00,0x00,0x00}},  //UP52 NUM 0x6058:00 =0
    {0x4C0, {0x02,0xD0,0x62,0x01,0x00,0x00,0x00,0x00}},  //UP53 NUM 0x62D0:01 =0
    {0x4C0, {0x02,0xD0,0x62,0x02,0x00,0x00,0x00,0x00}},  //UP54 NUM 0x62D0:02 =0
    {0x4C0, {0x02,0xD0,0x62,0x03,0x00,0x00,0x00,0x00}},  //UP55 NUM 0x62D0:03 =0
    {0x4C0, {0x02,0xD1,0x62,0x01,0x00,0x00,0x00,0x00}},  //UP56 NUM 0x62D1:01 =0
    {0x4C0, {0x02,0xD1,0x62,0x02,0x00,0x00,0x00,0x00}},  //UP57 NUM 0x62D1:02 =0
    {0x4C0, {0x02,0xD1,0x62,0x03,0x00,0x00,0x00,0x00}},  //UP58 NUM 0x62D1:03 =0
    {0x4C0, {0x01,0x22,0x63,0x00,0x7D,0x00,0x00,0x00}},  //UP59 NUM 0x6322:00 =125
    {0x4C0, {0x01,0x23,0x63,0x00,0x7D,0x00,0x00,0x00}},  //UP60 NUM 0x6323:00 =125
    {0x4C0, {0x02,0x72,0x63,0x00,0x00,0x00,0x00,0x00}},  //UP61 NUM 0x6372:00 =0

    //3)结束帧 ID=0x4C0  [FF FF 37 3D 00 7E 00 00]
    {0x4C0, {0xFF,0xFF,0x37,0x3D,0x7E,0x00,0x00,0x00}},
};

// 心跳包
static std::vector<TestFrame> kHeartTestCmd = {
    // {0x740, {0x01,0x00,0x00,0x00,0x40,0x00,0x00,0x00}}  // 心跳包
    {0x4C0, {0x01,0x00,0x20,0x02,0x0B,0x00,0x00,0x00}},  // 心跳包
    {0x4C0, {0x01,0x02,0x20,0x03,0x40,0x00,0x00,0x00}}  // 心跳包
};

// ===================== 测试用报警帧 =====================
// 说明: 除 0x3110(电源电压过低) 来自你的规范外,
//       其余故障码为自造测试值, 含义以注释为准
static std::vector<TestFrame> kTestWarningFrames = {
    // 1. 0x3110 电源电压过低 | 主电源 | 0x05(Bit0通用+Bit2电压) | 4次 | 1级(A)
    {0x0C0, {0x10, 0x31, 0x05, 0x30, 0x04, 0x00, 0x00, 0x00}},
    // 2. 0x3110 电源电压过低 | 主电源 | 12次 | 2级(B)+仍激活
    {0x0C0, {0x10, 0x31, 0x05, 0x30, 0x0C, 0x00, 0x05, 0x00}},
    // 3. 0x3111 电源电压过高 | 主电源 | 21次 | 4级(D)+仍激活
    {0x0C0, {0x11, 0x31, 0x05, 0x30, 0x15, 0x00, 0x07, 0x00}},
    // 4. 0x2320 驱动通道过流 | TLE通道1 | 0x03(通用+电流) | 2次 | 3级(C)+激活+降额
    {0x0C0, {0x20, 0x23, 0x03, 0x01, 0x02, 0x00, 0x0E, 0x00}},
    // 5. 0x2321 驱动通道短路 | TLE通道2 | 0x03 | 1次 | 2级(B)+仍激活
    {0x0C0, {0x21, 0x23, 0x03, 0x02, 0x01, 0x00, 0x05, 0x00}},
    // 6. 0x2322 驱动通道开路 | TLE通道3 | 0x03 | 1次 | 1级(A)   ← reg 由0x01改为0x03
    {0x0C0, {0x22, 0x23, 0x03, 0x03, 0x01, 0x00, 0x00, 0x00}},
    // 7. 0x4310 驱动通道过温 | TLE通道4 | 0x09(通用+温度) | 3次 | 2级(B)  ← 码由0x2330改为0x4310
    {0x0C0, {0x10, 0x43, 0x09, 0x04, 0x03, 0x00, 0x01, 0x00}},
    // 8. 0x2440 PWM输出故障 | PWM通道1 | 0x03 | 6次 | 3级(C)+仍激活  ← reg 由0x01改为0x03
    {0x0C0, {0x40, 0x24, 0x03, 0x05, 0x06, 0x00, 0x06, 0x00}},
    // 9. 0x2441 PWM占空比异常 | PWM通道2 | 0x03 | 2次 | 1级(A)       ← reg 由0x01改为0x03
    {0x0C0, {0x41, 0x24, 0x03, 0x06, 0x02, 0x00, 0x00, 0x00}},
    // 10. 0x5110 LVDT信号丢失 | LVDT位移传感器 | 0x11(通用+硬件) | 8次 | 2级(B)+仍激活 ← reg 0x01→0x11
    {0x0C0, {0x10, 0x51, 0x11, 0x10, 0x08, 0x00, 0x05, 0x00}},
    // 11. 0x4220 NTC超温 | NTC温度传感器 | 0x09 | 10次 | 3级(C)+激活+降额 ← 码由0x5220改为0x4220
    {0x0C0, {0x20, 0x42, 0x09, 0x20, 0x0A, 0x00, 0x0E, 0x00}},
    // 12. 0x8410 CAN通信中断 | CAN通信 | 0x41(通用+通信) | 1次 | 4级(D)+仍激活 ← 码0x5410→0x8410, reg 0x11→0x41
    {0x0C0, {0x10, 0x84, 0x41, 0x40, 0x01, 0x00, 0x07, 0x00}},
    // 13. 0x6090 整机软件故障 | 整机 | 0x21(通用+软件) | 1次 | 2级(B)
    {0x0C0, {0x90, 0x60, 0x21, 0x00, 0x01, 0x00, 0x01, 0x00}},
    // 14. ★故意错误帧: 0x55AA是0x5xxx硬件类, 但reg=0x80(液压位)且缺Bit0 → 应触发2条协议自校验警告;
    //     通道0x7F测未知通道default分支, 码不在表内测"硬件类未知故障码"回退 | 9次 | 4级+激活+降额
    {0x0C0, {0xAA, 0x55, 0x80, 0x7F, 0x09, 0x00, 0x0F, 0x00}},
    // 15. 0x7ABC 未定义大类(0x7无对应位) | 整机 | 0x01(仅Bit0) | 1次 | 3级(C) → 测"未定义大类"回退, 不应有警告
    {0x0C0, {0xBC, 0x7A, 0x01, 0x00, 0x01, 0x00, 0x02, 0x00}},
    // 16. 全0 = 故障复位帧 (按协议只打日志, 不弹窗)
    {0x0C0, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}
};

// ---------- 16 进制格式化: 0x3110 这种格式 ----------
inline QString hex16(quint16 v, int width = 4) {
    return QStringLiteral("0x") + QString::number(v, 16)
                                 .rightJustified(width, QChar('0')).toUpper();
}
inline QString hex8(quint8 v) {
    return QStringLiteral("0x") + QString::number(v, 16)
                                 .rightJustified(2, QChar('0')).toUpper();
}

// ---------- 故障码大类 (高 4 位, 与 9.9.3 错误寄存器一一对应) ----------
inline QString faultClassText(quint16 code) {
    switch (code >> 12) {
    case 0x1: return QStringLiteral("通用类");
    case 0x2: return QStringLiteral("驱动/电流类");
    case 0x3: return QStringLiteral("电源电压类");
    case 0x4: return QStringLiteral("温度类");
    case 0x5: return QStringLiteral("硬件类");
    case 0x6: return QStringLiteral("软件类");
    case 0x8: return QStringLiteral("CAN 通信类");
    case 0x9: return QStringLiteral("液压/应用类");
    default:  return QStringLiteral("未定义大类");
    }
}

// ---------- 具体故障码表 ----------
// ★ 目前只有 0x3110(电源电压过低) 是规范确认的;
//   其余为占位, 联调时请按规范里的故障码表逐条替换, 结构不用动
inline QString faultCodeText(quint16 code) {
    switch (code) {
    // 电源电压类 0x3xxx
    case 0x3110: return QStringLiteral("电源电压过低");
    case 0x3111: return QStringLiteral("电源电压过高");
    // 驱动/电流类 0x2xxx
    case 0x2310: return QStringLiteral("驱动通道过流");
    case 0x2311: return QStringLiteral("驱动通道短路");
    case 0x2312: return QStringLiteral("驱动通道开路");
    case 0x2410: return QStringLiteral("PWM 输出过流");
    // 温度类 0x4xxx
    case 0x4310: return QStringLiteral("驱动通道过温");
    case 0x4210: return QStringLiteral("NTC 温度超限");
    // 硬件类 0x5xxx
    case 0x5110: return QStringLiteral("LVDT 信号丢失");
    case 0x5210: return QStringLiteral("POST 自检失败");
    // 软件类 0x6xxx
    case 0x6100: return QStringLiteral("参数配置/写入错误");
    // CAN 通信类 0x8xxx
    case 0x8110: return QStringLiteral("CAN 心跳丢失");
    case 0x8210: return QStringLiteral("通信协议不兼容");
    // 液压/应用类 0x9xxx
    case 0x9110: return QStringLiteral("液压油位过低");
    case 0x9120: return QStringLiteral("液压压力超限");
    default:     return QString();   // 未知码 → 外层回退为显示大类
    }
}

// ---------- 0x1001 错误寄存器 (严格按 9.9.3) ----------
inline QStringList errorRegisterBits(quint8 reg) {
    QStringList bits;
    if (reg & 0x01) bits << QStringLiteral("Bit0 通用错误");
    if (reg & 0x02) bits << QStringLiteral("Bit1 电流错误(0x2xxx)");
    if (reg & 0x04) bits << QStringLiteral("Bit2 电压错误(0x3xxx)");
    if (reg & 0x08) bits << QStringLiteral("Bit3 温度错误(0x4xxx)");
    if (reg & 0x10) bits << QStringLiteral("Bit4 硬件错误(0x5xxx)");
    if (reg & 0x20) bits << QStringLiteral("Bit5 软件错误(0x6xxx)");
    if (reg & 0x40) bits << QStringLiteral("Bit6 通信错误(0x8xxx)");
    if (reg & 0x80) bits << QStringLiteral("Bit7 液压/应用错误(0x9xxx)");
    return bits;
}

// ---------- 故障大类 ↔ 0x1001 对应位 (用于协议自校验) ----------
inline quint8 classToRegBit(quint16 code) {
    switch (code >> 12) {
    case 0x2: return 0x02;
    case 0x3: return 0x04;
    case 0x4: return 0x08;
    case 0x5: return 0x10;
    case 0x6: return 0x20;
    case 0x8: return 0x40;
    case 0x9: return 0x80;
    default:  return 0x00;
    }
}

struct SettingRange {
    QString index_;
    QString sub_idx_;
    int min_value_;    
    int max_value_;
};

static std::vector<SettingRange> kSettingRanges {
    {"0x2002", "0x03", 0, 127},
    {"0x2003", "0x02", 83, 162},
    {"0x2003", "0x03", 83, 162},
    {"0x2003", "0x04", 83, 162},
    {"0x2003", "0x05", 83, 162},
    {"0x2003", "0x06", 83, 162},

    {"0x2003", "0x0A", 5, 83},
    {"0x2003", "0x0B", 5, 83},
    {"0x2003", "0x0C", 5, 83},
    {"0x2003", "0x0D", 5, 83},
    {"0x2003", "0x0E", 5, 83}
};

static bool IsValueOutOfRanged(QString index, QString sub_idx, int value) {
    auto it = std::find_if(kSettingRanges.begin(), kSettingRanges.end(), [&](SettingRange range){
        return (range.index_ == index) && (range.sub_idx_ == sub_idx);
    });

    if(it != kSettingRanges.end()) {
        auto range = (*it);
        return (range.min_value_ > value) || (value > range.max_value_);
    }

    return false;
}

static QString GetOutOfRangeString(QString index, QString sub_idx, bool sl = false) {
    auto it = std::find_if(kSettingRanges.begin(), kSettingRanges.end(), [&](SettingRange range){
        return (range.index_ == index) && (range.sub_idx_ == sub_idx);
    });

    if(it != kSettingRanges.end()) {
        auto range = (*it);
        if(sl) {
            return QString("(0x%1, 0x%2)").arg(QString::number(range.min_value_, 16)).arg(QString::number(range.max_value_, 16));
        }
        return QString("(%1, %2)").arg(range.min_value_).arg(range.max_value_);
    }

    return "";
}

static int limitCurrent1 = 0;
static int limitCurrent2 = 0;

static void gSetLimitCurrent(int side, int value) {
    if(side == 1) {
        limitCurrent1 = value;
    } else if(side == 2) {
        limitCurrent2 = value;
    }
}

static bool IsOutOfRangeLimitCur(int side, int value) {
    if(side == 1) {
        return limitCurrent1 < value;
    } else if(side == 2) {
        return limitCurrent2 < value;
    }
    return false;
}

static int GetLimitCur(int side) {
    if(side == 1) {
        return limitCurrent1;
    } else if(side == 2) {
        return limitCurrent2;
    }
    return 0;
}

static const QSet<QString> targetSet = {
    "0x12",
    "0x13",
    "0x14",
    "0x15",
    "0x16",
    "0x1A",
    "0x1B",
    "0x1C",
    "0x1D",
    "0x1E"
};

inline bool IsCurSubIndex(QString index) {
    return targetSet.contains(index);
}

inline int GetIndexSide(QString index) {
    if(index == "0x12" || index == "0x13"|| index == "0x14"|| index == "0x15"|| index == "0x16") {
        return 1;
    } else if (index == "0x1A" || index == "0x1B" || index == "0x1C" || index == "0x1D" || index == "0x1E") {
        return 2;
    }
    return 0;
}

#endif // _UTILS_H