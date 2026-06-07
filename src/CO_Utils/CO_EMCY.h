#ifndef CO_EMCY_HPP
#define CO_EMCY_HPP

#include "CO_ObjectDictionary.h"
#include "CanDriver.h"
#include "CO_NMT.h"

class CO_EMCY {
public:
    CO_EMCY(CO_ObjectDictionary& od, CanDriver& can, uint8_t nodeId);
    void setError(uint16_t errorCode, uint8_t errorRegisterMask);
    void clearError();
    void checkAndSend();  // 检查错误寄存器变化并发送
private:
    void sendEMCY(uint16_t errorCode, uint8_t errorReg, const std::vector<uint8_t>& manufData);
    CO_ObjectDictionary& od_;
    CanDriver& can_;
    uint8_t nodeId_;
    uint8_t lastErrorReg_;
    uint16_t lastErrorCode_;
};

#endif