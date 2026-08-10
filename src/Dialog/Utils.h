#ifndef _UTILS_H
#define _UTILS_H

#include <vector>
#include <cstdint>
#include <QDebug>
#include <iostream>

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

    // 3. 【小端模式】填充第5、6字节（下标4、5）
    cmd[4] = static_cast<uint8_t>(value & 0xFF);        // 低字节 0xE8
    cmd[5] = static_cast<uint8_t>((value >> 8) & 0xFF); // 高字节 0x03

    return cmd;
}

static std::vector<uint8_t> SetPIDCMDValue(const std::array<uint8_t, 8> command, double val) {
    uint16_t value = static_cast<uint16_t>(val * 1000); 
    std::cout << "PID value: " << value << std::endl;

    // 2. 复制模板生成待发送指令
    std::vector<uint8_t> cmd(command.begin(), command.end());

    // 3. 【小端模式】填充第5、6字节（下标4、5）
    cmd[4] = static_cast<uint8_t>(value & 0xFF);        // 低字节 0xE8
    cmd[5] = static_cast<uint8_t>((value >> 8) & 0xFF); // 高字节 0x03

    return cmd;
}


static void PrintCmd(const std::vector<uint8_t> &cmd, std::string prefix = "") {
    std::cout << prefix << " cmd: ";
    for (auto byte : cmd) {
        std::cout << QString("0x%1").arg(byte, 2, 16, QChar('0')).toUpper().toStdString() << " ";
    }
    std::cout << std::endl;
}
#endif // _UTILS_H