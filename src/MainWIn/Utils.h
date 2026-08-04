#ifndef _UTILS_H
#define _UTILS_H

#include <vector>
#include <cstdint>
#include <QDebug>

// 提取SDO响应中第5、6字节（即 data[4] 和 data[5]）
uint16_t extractWordFromSdoResponse(const std::vector<uint8_t>& data) {
    if (data.size() < 6) {
        qWarning() << "SDO response data too short, size:" << data.size();
        return 0;
    }
    // 小端序：第5字节为低字节，第6字节为高字节
    uint16_t value = static_cast<uint16_t>(data[4]) | 
                     (static_cast<uint16_t>(data[5]) << 8);
    return value;
}

// 提取SDO响应中第5-8字节的32位值
uint32_t extractDwordFromSdoResponse(const std::vector<uint8_t>& data) {
    if (data.size() < 8) {
        qWarning() << "SDO response data too short, size:" << data.size();
        return 0;
    }
    uint32_t value = static_cast<uint32_t>(data[4]) |
                     (static_cast<uint32_t>(data[5]) << 8) |
                     (static_cast<uint32_t>(data[6]) << 16) |
                     (static_cast<uint32_t>(data[7]) << 24);
    return value;
}

#endif // _UTILS_H