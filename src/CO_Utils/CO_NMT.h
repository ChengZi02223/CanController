#ifndef CO_NMT_HPP
#define CO_NMT_HPP

#include "CanDriver.h"
#include <cstdint>
#include <functional>

enum class NmtState : uint8_t {
    INITIALIZING = 0,
    PRE_OPERATIONAL = 127,
    OPERATIONAL = 5,
    STOPPED = 4
};

class CO_NMT {
public:
    CO_NMT(CanDriver& can, uint8_t nodeId);
    bool processCommand(const can_frame& frame);
    NmtState getState() const { return state_; }
    void setStateChangeCallback(std::function<void(NmtState, NmtState)> cb);
    void setInitialState(NmtState state) { state_ = state; }

private:
    void changeState(NmtState newState);
    CanDriver& can_;
    uint8_t nodeId_;
    NmtState state_;
    std::function<void(NmtState, NmtState)> stateChangeCallback_;
};

#endif