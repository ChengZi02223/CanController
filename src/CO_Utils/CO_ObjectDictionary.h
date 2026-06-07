#ifndef CO_OBJECT_DICTIONARY_HPP
#define CO_OBJECT_DICTIONARY_HPP

#include <cstdint>
#include <map>
#include <vector>
#include <functional>
#include <cstring>

enum class ODDataType : uint8_t {
    UINT8, INT8, UINT16, INT16, UINT32, INT32, BOOLEAN, STRING, DOMAIN
};

struct ODEntry {
    uint16_t index;
    uint8_t subindex;
    ODDataType dataType;
    uint32_t dataLength;
    std::vector<uint8_t> data;
    uint8_t access;  // 0=无,1=只读,2=只写,3=读写
    std::function<void(const std::vector<uint8_t>&)> onChange;
    
    // 默认构造函数
    ODEntry() 
        : index(0), subindex(0), dataType(ODDataType::UINT8), 
          dataLength(0), access(0) {}

    ODEntry(uint16_t idx, uint8_t subidx, ODDataType type, uint32_t len, uint8_t acc)
        : index(idx), subindex(subidx), dataType(type), dataLength(len), access(acc) {
        data.resize(len, 0);
    }
    
    template<typename T>
    T getValue() const {
        T val = 0;
        if (data.size() >= sizeof(T)) memcpy(&val, data.data(), sizeof(T));
        return val;
    }
    
    template<typename T>
    void setValue(T val) {
        if (data.size() >= sizeof(T)) {
            memcpy(data.data(), &val, sizeof(T));
            if (onChange) onChange(data);
        }
    }
};

class CO_ObjectDictionary {
public:
    CO_ObjectDictionary();
    
    void addEntry(uint16_t index, uint8_t subindex, ODDataType type, uint32_t length, uint8_t access,
                  const std::vector<uint8_t>& initialData = {});
    ODEntry* getEntry(uint16_t index, uint8_t subindex);
    bool hasEntry(uint16_t index, uint8_t subindex) const;
    void dump() const;
    
private:
    std::map<uint32_t, ODEntry> entries_;
    uint32_t makeKey(uint16_t index, uint8_t subindex) const {
        return (static_cast<uint32_t>(index) << 8) | subindex;
    }
};

#endif