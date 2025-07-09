#ifndef SERVICE_EEPROM_H
#define SERVICE_EEPROM_H

#include <map>
#include <string>

#include <EEPROM.h>
#include <Arduino.h>

#include "../IService.h"
#include "../ServiceRegistry.h"

class EEPROMService : public IService {
public:
    EEPROMService(ServiceRegistry& registry, Scheduler& scheduler, const char* tag = "EEPROMService") 
        : registry(registry), TAG(tag), eepromSize(512), nextFreeAddr(0), isReady(false) {}

    const char* getTag() const override {
        return TAG;
    }

    void start() override {
        if (!EEPROM.begin(eepromSize)) {
            Serial.print(TAG); Serial.println(" failed to start EEPROM.");
            isReady = false;
            return;
        }
        nextFreeAddr = 0;
        entries.clear();
        Serial.print(TAG); Serial.println(" started EEPROM.");
        isReady = true;
    }

    void update(unsigned long delta_ms) override {
        delay(1); // still questionable but keeping your original code
    }

    unsigned long cycleTimeMs() const override {
        return 0; // never repeat
    }

    bool ready() const override {
        return isReady;
    }

    bool registerStaticEntry(const std::string& key, size_t size) {
        if (entries.count(key) > 0)
            return true;
        if (nextFreeAddr + size > eepromSize) {
            Serial.print(TAG); Serial.print(" - EEPROM overflow trying to register key: ");
            Serial.println(key.c_str());
            return false;
        }
        entries[key] = Entry{nextFreeAddr, size};
        Serial.print(TAG); Serial.print(" - Registered key ");
        Serial.print(key.c_str()); Serial.print(" at addr ");
        Serial.print(nextFreeAddr); Serial.print(" (");
        Serial.print(size); Serial.println(" bytes)");
        nextFreeAddr += size;
        return true;
    }

    template<typename T>
    bool write(const std::string& key, const T& val) {
        if (!checkKey(key, sizeof(T))) return false;
        EEPROM.put(entries[key].address, val);
        return EEPROM.commit();
    }

    template<typename T>
    bool read(const std::string& key, T& outVal) {
        if (!checkKey(key, sizeof(T))) return false;
        EEPROM.get(entries[key].address, outVal);
        return true;
    }

    void debugPrintUsage() {
        Serial.print(TAG); Serial.print(" - EEPROM used: ");
        Serial.print(nextFreeAddr); Serial.print(" / ");
        Serial.println(eepromSize);
    }

private:
    struct Entry {
        size_t address;
        size_t size;
    };

    ServiceRegistry& registry;
    const char* TAG;
    size_t eepromSize;
    size_t nextFreeAddr;
    std::map<std::string, Entry> entries;
    bool isReady;

    bool checkKey(const std::string& key, size_t size) {
        auto it = entries.find(key);
        if (it == entries.end()) {
            Serial.print(TAG); Serial.print(" - Unknown key: ");
            Serial.println(key.c_str());
            return false;
        }
        if (it->second.size < size) {
            Serial.print(TAG); Serial.print(" - Value too large for key: ");
            Serial.println(key.c_str());
            return false;
        }
        return true;
    }
};

#endif
