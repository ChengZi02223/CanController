#ifndef _UTILS_H
#define _UTILS_H

#include <vector>
#include <cstdint>
#include <QDebug>
#include <iostream>

#define ON_TEST_MODE
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
static std::vector<uint8_t> RampTimeCMDConfig(const std::array<uint8_t, 8> command, double time) {
    int bit = time / 20;

    std::vector<uint8_t> cmd(command.begin(), command.end());
    
    uint8_t value_8 = static_cast<uint8_t>(bit);

    cmd[4] = value_8;
    cmd[5] = value_8;
    cmd[6] = value_8;
    cmd[7] = value_8;

    return cmd;
}


static void PrintCmd(const uint32_t cobId, const std::vector<uint8_t> &cmd, QString prefix = "cmd") {
    // std::cout << prefix << "(" << QString("0x%1").arg(cobId, 2, 16, QChar('0')).toUpper().toStdString() << "): ";
    // for (auto byte : cmd) {
    //     std::cout << QString("%1").arg(byte, 2, 16, QChar('0')).toUpper().toStdString() << " ";
    // }
    // std::cout << std::endl;
    QString hexPayload;
    for (auto byte : cmd)
    {
        hexPayload += QString("%1 ").arg(byte,2,16,QLatin1Char('0')).toUpper();
    }
    qDebug().noquote() << prefix << "(0x" <<QString("%1):").arg(cobId,4,16,QLatin1Char('0')).toUpper() << hexPayload;
}

static bool CheckAnswerHead(const uint8_t data[8], const std::vector<uint8_t> &head) {
    if(head.size() !=4U) return false;
    return std::equal(head.begin(),
                      head.end(),
                      data);
}
#endif // _UTILS_H