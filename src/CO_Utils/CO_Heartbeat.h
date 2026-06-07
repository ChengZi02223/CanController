#ifndef CO_HEARTBEAT_HPP
#define CO_HEARTBEAT_HPP

#include "CanDriver.h"
#include "CO_NMT.h"
#include <atomic>
#include <thread>

class CO_Heartbeat {
public:
    CO_Heartbeat(CanDriver& can, uint8_t nodeId, CO_NMT& nmt);
    ~CO_Heartbeat();
    void start();
    void stop();
private:
    void run();
    void sendHeartbeat();
    CanDriver& can_;
    uint8_t nodeId_;
    CO_NMT& nmt_;
    std::atomic<bool> running_;
    std::thread thread_;
};

#endif