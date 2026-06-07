#include "CO_NMT.h"
#include <iostream>

CO_NMT::CO_NMT(CanDriver& can, uint8_t nodeId)
    : can_(can), nodeId_(nodeId), state_(NmtState::INITIALIZING) {}

bool CO_NMT::processCommand(const can_frame& frame) {
    if ((frame.can_id & 0x7FF) != 0x000) return false;
    uint8_t cs = frame.data[0];
    uint8_t target = frame.data[1];
    if (target != 0 && target != nodeId_) return false;
    
    switch (cs) {
        case 0x01: changeState(NmtState::OPERATIONAL); break;
        case 0x02: changeState(NmtState::STOPPED); break;
        case 0x80: changeState(NmtState::PRE_OPERATIONAL); break;
        case 0x81: case 0x82: changeState(NmtState::PRE_OPERATIONAL); break;
        default: return false;
    }
    return true;
}

void CO_NMT::setStateChangeCallback(std::function<void(NmtState, NmtState)> cb) {
    stateChangeCallback_ = cb;
}

void CO_NMT::changeState(NmtState newState) {
    if (state_ == newState) return;
    NmtState old = state_;
    state_ = newState;
    std::cout << "NMT state: " << (int)old << " -> " << (int)newState << std::endl;
    if (stateChangeCallback_) stateChangeCallback_(old, newState);
}