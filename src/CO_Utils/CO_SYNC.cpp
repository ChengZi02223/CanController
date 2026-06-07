#include "CO_SYNC.h"

CO_SYNC::CO_SYNC(CanDriver& can, CO_NMT& nmt, CO_PDO& pdo)
    : can_(can), nmt_(nmt), pdo_(pdo) {}

void CO_SYNC::processSync(const can_frame& frame) {
    if ((frame.can_id & 0x7FF) != 0x80) return;
    pdo_.triggerSyncTPDOs();
}