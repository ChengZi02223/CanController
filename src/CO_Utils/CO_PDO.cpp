#include "CO_PDO.h"
#include "CanDriver.h"
#include <cstring>
#include <iostream>

CO_PDO::CO_PDO(CO_ObjectDictionary& od,  CO_NMT& nmt, uint8_t nodeId)
    : od_(od), nmt_(nmt), nodeId_(nodeId) {
    // 硬编码示例：一个 TPDO（索引0x1800+0，映射0x1A00）
    TPDO tpdo0;
    tpdo0.cobId = 0x180 + nodeId;
    tpdo0.transmissionType = 254;  // 事件触发
    tpdo0.mappings.push_back({0x2000, 0x00, 16}); // 映射到对象2000:00（16位）
    tpdo0.lastData.resize(2, 0);
    tpdoList_.push_back(tpdo0);
    
    // 一个同步 TPDO (transmissionType=1)
    TPDO tpdo1;
    tpdo1.cobId = 0x280 + nodeId;
    tpdo1.transmissionType = 1;    // 同步触发
    tpdo1.mappings.push_back({0x2001, 0x00, 32});
    tpdo1.lastData.resize(4, 0);
    tpdoList_.push_back(tpdo1);
    
    // 一个 RPDO (COB-ID 0x200+nodeId)
    RPDO rpdo0;
    rpdo0.cobId = 0x200 + nodeId;
    rpdo0.mappings.push_back({0x2002, 0x00, 16});
    rpdoList_.push_back(rpdo0);
    
    // 确保对象字典中存在这些对象
    if (!od_.hasEntry(0x2000,0x00)) od_.addEntry(0x2000,0,ODDataType::UINT16,2,3,{0,0});
    if (!od_.hasEntry(0x2001,0x00)) od_.addEntry(0x2001,0,ODDataType::UINT32,4,3,{0,0,0,0});
    if (!od_.hasEntry(0x2002,0x00)) od_.addEntry(0x2002,0,ODDataType::UINT16,2,3,{0,0});
}

void CO_PDO::processRPDO(const can_frame& frame) {
    if (nmt_.getState() != NmtState::OPERATIONAL) return;
    uint16_t cob = frame.can_id & 0x7FF;
    for (auto& rpdo : rpdoList_) {
        if (rpdo.cobId == cob) {
            // 解包数据并按映射写入OD
            int bitPos = 0;
            for (const auto& map : rpdo.mappings) {
                int byteOffset = bitPos / 8;
                int bitOffset = bitPos % 8;
                if (byteOffset + (map.bitLength+7)/8 > frame.can_dlc) break;
                uint32_t value = 0;
                memcpy(&value, frame.data + byteOffset, (map.bitLength+7)/8);
                // 简单写入（需按位对齐，此处简化）
                ODEntry* entry = od_.getEntry(map.index, map.subindex);
                if (entry && entry->data.size() >= (map.bitLength+7)/8) {
                    memcpy(entry->data.data(), &value, (map.bitLength+7)/8);
                    if (entry->onChange) entry->onChange(entry->data);
                }
                bitPos += map.bitLength;
            }
            break;
        }
    }
}

void CO_PDO::processEventDriven() {
    if (nmt_.getState() != NmtState::OPERATIONAL) return;
    for (auto& tpdo : tpdoList_) {
        if (tpdo.transmissionType == 254) { // 事件触发
            // 收集当前映射对象的值
            std::vector<uint8_t> current;
            for (const auto& map : tpdo.mappings) {
                ODEntry* entry = od_.getEntry(map.index, map.subindex);
                if (entry) {
                    int bytes = (map.bitLength+7)/8;
                    current.insert(current.end(), entry->data.begin(), entry->data.begin()+bytes);
                }
            }
            if (current != tpdo.lastData) {
                tpdo.lastData = current;
                sendTPDO(tpdo);
            }
        }
    }
}

void CO_PDO::triggerSyncTPDOs() {
    if (nmt_.getState() != NmtState::OPERATIONAL) return;
    for (auto& tpdo : tpdoList_) {
        if (tpdo.transmissionType >= 1 && tpdo.transmissionType <= 240) {
            sendTPDO(tpdo);
        }
    }
}

void CO_PDO::sendTPDO(const TPDO& tpdo) {
    can_frame frame;
    frame.can_id = tpdo.cobId;
    frame.can_dlc = 0;
    std::vector<uint8_t> outData;
    for (const auto& map : tpdo.mappings) {
        ODEntry* entry = od_.getEntry(map.index, map.subindex);
        if (entry) {
            int bytes = (map.bitLength+7)/8;
            outData.insert(outData.end(), entry->data.begin(), entry->data.begin()+bytes);
        }
    }
    frame.can_dlc = std::min(8, (int)outData.size());
    memcpy(frame.data, outData.data(), frame.can_dlc);
    CanDriver::GetInstance()->send(frame);
}