#include "CO_EMCY.h"
#include <cstring>

CO_EMCY::CO_EMCY(CO_ObjectDictionary& od, CanDriver& can, uint8_t nodeId)
    : od_(od), can_(can), nodeId_(nodeId), lastErrorReg_(0), lastErrorCode_(0) {}

void CO_EMCY::setError(uint16_t errorCode, uint8_t errorRegisterMask) {
    ODEntry* errReg = od_.getEntry(0x1001, 0);
    if (errReg) {
        uint8_t val = errReg->getValue<uint8_t>();
        val |= errorRegisterMask;
        errReg->setValue(val);
    }
    lastErrorCode_ = errorCode;
    sendEMCY(errorCode, errorRegisterMask, {0,0,0,0,0});
}

void CO_EMCY::clearError() {
    ODEntry* errReg = od_.getEntry(0x1001, 0);
    if (errReg) errReg->setValue((uint8_t)0);
    sendEMCY(0, 0, {0,0,0,0,0});
}

void CO_EMCY::checkAndSend() {
    ODEntry* errReg = od_.getEntry(0x1001, 0);
    if (!errReg) return;
    uint8_t current = errReg->getValue<uint8_t>();
    if (current != lastErrorReg_) {
        lastErrorReg_ = current;
        if (current != 0) {
            // 实际错误码应来自其他诊断，此处用默认
            sendEMCY(0x1000, current, {0,0,0,0,0});
        } else {
            sendEMCY(0, 0, {0,0,0,0,0});
        }
    }
}

void CO_EMCY::sendEMCY(uint16_t errorCode, uint8_t errorReg, const std::vector<uint8_t>& manufData) {
    can_frame frame;
    frame.can_id = 0x80 + nodeId_;
    frame.can_dlc = 8;
    frame.data[0] = errorCode & 0xFF;
    frame.data[1] = (errorCode >> 8) & 0xFF;
    frame.data[2] = errorReg;
    for (int i=0; i<5 && i<manufData.size(); i++) frame.data[3+i] = manufData[i];
    can_.send(frame);
}