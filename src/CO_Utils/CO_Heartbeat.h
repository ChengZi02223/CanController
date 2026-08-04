#ifndef CO_HEARTBEAT_HPP
#define CO_HEARTBEAT_HPP

#include "CO_NMT.h"
#include <atomic>
#include <thread>

class CO_Heartbeat {
public:
    CO_Heartbeat(uint8_t nodeId, CO_NMT& nmt);
    ~CO_Heartbeat();
    void start();
    void stop();
private:
    void run();
    void sendHeartbeat();
    uint8_t nodeId_;
    CO_NMT& nmt_;
    std::atomic<bool> running_;
    std::thread thread_;
};

#endif