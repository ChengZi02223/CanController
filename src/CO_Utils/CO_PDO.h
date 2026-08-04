#ifndef CO_PDO_HPP
#define CO_PDO_HPP

#include "CO_ObjectDictionary.h"
#include "CO_NMT.h"
#include <vector>
#include <cstdint>

struct PDO_Mapping {
    uint16_t index;
    uint8_t subindex;
    uint8_t bitLength;
};

struct TPDO {
    uint16_t cobId;
    uint8_t transmissionType;  // 0=同步, 1-240=同步周期, 254=事件, 255=厂商
    std::vector<PDO_Mapping> mappings;
    std::vector<uint8_t> lastData; // 用于检测变化（事件触发）
};

struct RPDO {
    uint16_t cobId;
    std::vector<PDO_Mapping> mappings;
};

class CO_PDO {
public:
    CO_PDO(CO_ObjectDictionary& od, CO_NMT& nmt, uint8_t nodeId);
    void processRPDO(const can_frame& frame);
    void processEventDriven();   // 定期调用，检查事件触发TPDO
    void triggerSyncTPDOs();     // 收到SYNC时调用
    
private:
    void buildTPDO(uint8_t tpdoNum, TPDO& tpdo);
    void buildRPDO(uint8_t rpdoNum, RPDO& rpdo);
    void sendTPDO(const TPDO& tpdo);
    
    CO_ObjectDictionary& od_;
    CO_NMT& nmt_;
    uint8_t nodeId_;
    std::vector<TPDO> tpdoList_;
    std::vector<RPDO> rpdoList_;
};

#endif