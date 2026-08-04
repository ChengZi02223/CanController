#include "CO_Heartbeat.h"
#include "CanDriver.h"
#include <chrono>

CO_Heartbeat::CO_Heartbeat( uint8_t nodeId, CO_NMT& nmt)
    : nodeId_(nodeId), nmt_(nmt), running_(false) {}

CO_Heartbeat::~CO_Heartbeat() { stop(); }

void CO_Heartbeat::start() {
    if (running_) return;
    running_ = true;
    thread_ = std::thread(&CO_Heartbeat::run, this);
}

void CO_Heartbeat::stop() {
    if (!running_) return;
    running_ = false;
    if (thread_.joinable()) thread_.join();
}

void CO_Heartbeat::run() {
    while (running_) {
        sendHeartbeat();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void CO_Heartbeat::sendHeartbeat() {
    NmtState state = nmt_.getState();
    uint8_t code = 0;
    switch (state) {
        case NmtState::INITIALIZING: code = 0; break;
        case NmtState::PRE_OPERATIONAL: code = 127; break;
        case NmtState::OPERATIONAL: code = 5; break;
        case NmtState::STOPPED: code = 4; break;
    }
    can_frame frame;
    frame.can_id = 0x700 + nodeId_;
    frame.can_dlc = 1;
    frame.data[0] = code;
     CanDriver::GetInstance()->send(frame);
}