#include "CO_ObjectDictionary.h"
#include <iostream>
#include <iomanip>

CO_ObjectDictionary::CO_ObjectDictionary() {}

void CO_ObjectDictionary::addEntry(uint16_t index, uint8_t subindex, ODDataType type,
                                   uint32_t length, uint8_t access,
                                   const std::vector<uint8_t>& initialData) {
    uint32_t key = makeKey(index, subindex);
    auto result = entries_.emplace(key, ODEntry(index, subindex, type, length, access));
    if (result.second && !initialData.empty()) {
        ODEntry& entry = result.first->second;
        size_t copyLen = std::min(initialData.size(), entry.data.size());
        std::copy(initialData.begin(), initialData.begin() + copyLen, entry.data.begin());
    }
}

ODEntry* CO_ObjectDictionary::getEntry(uint16_t index, uint8_t subindex) {
    auto it = entries_.find(makeKey(index, subindex));
    return (it != entries_.end()) ? &it->second : nullptr;
}

bool CO_ObjectDictionary::hasEntry(uint16_t index, uint8_t subindex) const {
    return entries_.find(makeKey(index, subindex)) != entries_.end();
}

void CO_ObjectDictionary::dump() const {
    std::cout << "Object Dictionary:\n";
    for (const auto& pair : entries_) {
        const auto& e = pair.second;
        std::cout << std::hex << "  0x" << e.index << ":" << (int)e.subindex
                  << " type=" << (int)e.dataType << " len=" << std::dec << e.dataLength
                  << " access=" << (int)e.access << std::endl;
    }
}