#include "CO_SDO.h"
#include "CanDriver.h"
#include <cstring>
#include <iostream>

CO_SDO::CO_SDO(CO_ObjectDictionary& od, uint8_t nodeId)
    : od_(od), nodeId_(nodeId) {
    sdoRxCobId_ = 0x600 + nodeId;
    sdoTxCobId_ = 0x580 + nodeId;
}

bool CO_SDO::processRequest(const can_frame& frame) {
    if ((frame.can_id & 0x7FF) != sdoRxCobId_) return false;
    const uint8_t* d = frame.data;
    uint8_t cc = d[0] >> 5;
    uint16_t idx = d[1] | (d[2] << 8);
    uint8_t sub = d[3];
    
    if (cc == 0x20) { // Write
        uint8_t size = (d[0] & 0x0F) + 1;
        std::vector<uint8_t> value(d+4, d+4+size);
        ODEntry* entry = od_.getEntry(idx, sub);
        if (!entry) { sendAbort(idx, sub, 0x06020000); return true; }
        if ((entry->access & 0x02) == 0) { sendAbort(idx, sub, 0x06010002); return true; }
        if (value.size() > entry->data.size()) { sendAbort(idx, sub, 0x06070010); return true; }
        std::copy(value.begin(), value.end(), entry->data.begin());
        if (entry->onChange) entry->onChange(entry->data);
        // 写响应
        uint8_t resp[8] = {0x60, (uint8_t)(idx&0xFF), (uint8_t)(idx>>8), sub, 0,0,0,0};
        can_frame f; f.can_id = sdoTxCobId_; f.can_dlc = 8; memcpy(f.data, resp, 8);
        CanDriver::GetInstance()->send(f);
    } else if (cc == 0x40) { // Read
        ODEntry* entry = od_.getEntry(idx, sub);
        if (!entry) { sendAbort(idx, sub, 0x06020000); return true; }
        if ((entry->access & 0x01) == 0) { sendAbort(idx, sub, 0x06010001); return true; }
        uint8_t resp[8] = {
            static_cast<uint8_t>(0x40 | (entry->dataLength << 2) | 3),
            static_cast<uint8_t>(idx & 0xFF),
            static_cast<uint8_t>((idx >> 8) & 0xFF),
            sub,
            0, 0, 0, 0
        };
        std::copy(entry->data.begin(), entry->data.end(), resp+4);
        can_frame f; f.can_id = sdoTxCobId_; f.can_dlc = 8; memcpy(f.data, resp, 8);
        CanDriver::GetInstance()->send(f);
    } else {
        sendAbort(idx, sub, 0x06010000);
    }
    return true;
}

void CO_SDO::sendAbort(uint16_t index, uint8_t subindex, uint32_t abortCode) {
    uint8_t data[8] = {0x80, (uint8_t)(index&0xFF), (uint8_t)(index>>8), subindex};
    memcpy(data+4, &abortCode, 4);
    can_frame f; f.can_id = sdoTxCobId_; f.can_dlc = 8; memcpy(f.data, data, 8);
    CanDriver::GetInstance()->send(f);
}