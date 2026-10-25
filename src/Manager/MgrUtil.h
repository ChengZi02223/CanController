#ifndef _MSG_UTIL_H_
#define _MSG_UTIL_H_

#include "CanManager.h"
#include "can_cmd.h"

#define CAN_MGR_START CanManager::GetInstance()->Start()

inline bool CanCheckAnswer(uint32_t id) {
    return id == GetRPDOCobId() || id == GetSDOCobId() || id == GetRPDO2CobId();
}

/// TODO:
inline uint32_t CanReceiveAnswer(uint32_t id) {
    if(id == GetRPDOCobId()) {
        return 0x2c0; 
    } else if (id == GetRPDO2CobId()) {
        return 0x3c0;
    } else if (id == GetSDOCobId()) {
        return 0x5c0;
    } 
    return 0x00;
}

inline void CAN_MGR_PUSH_CMD(uint32_t id, const std::vector<uint8_t>& cmd) {
    auto manager = CanManager::GetInstance();
    manager->PushCommand(id, cmd, CanCheckAnswer(id));
}

inline void CAN_MGR_PUSH_CMD(uint32_t id, const uint8_t data[8], uint8_t len = 8)
{
    // 限制最大长度8，防止越界
    const uint8_t realLen = std::min(len, (uint8_t)8);
    std::vector<uint8_t> cmd(data, data + realLen);
    CanManager::GetInstance()->PushCommand(id, cmd, CanCheckAnswer(id));
}

inline void CAN_MGR_PUSH_CMDS(const std::vector<CanCmdItem>& items) {
    CanManager::GetInstance()->PushCommands(items);
}


#endif // _MSG_UTIL_H_