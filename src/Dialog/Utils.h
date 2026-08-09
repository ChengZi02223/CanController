#ifndef _UTILS_H
#define _UTILS_H

#include <vector>
#include <cstdint>
#include <QDebug>

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

#endif // _UTILS_H