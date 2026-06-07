#ifndef CO_SDO_HPP
#define CO_SDO_HPP

#include "CO_ObjectDictionary.h"
#include "CanDriver.h"

class CO_SDO {
public:
    CO_SDO(CO_ObjectDictionary& od, CanDriver& can, uint8_t nodeId);
    bool processRequest(const can_frame& frame);
private:
    void sendAbort(uint16_t index, uint8_t subindex, uint32_t abortCode);
    CO_ObjectDictionary& od_;
    CanDriver& can_;
    uint8_t nodeId_;
    uint16_t sdoRxCobId_;
    uint16_t sdoTxCobId_;
};

#endif