#ifndef CO_SYNC_HPP
#define CO_SYNC_HPP

#include "CO_NMT.h"
#include "CO_PDO.h"
#include <functional>

class CO_SYNC {
public:
    CO_SYNC( CO_NMT& nmt, CO_PDO& pdo);
    void processSync(const can_frame& frame);
private:
    CO_NMT& nmt_;
    CO_PDO& pdo_;
};

#endif