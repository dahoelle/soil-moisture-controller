#ifndef SERVICE_BOILERPLATE_H
#define SERVICE_BOILERPLATE_H

#include <Arduino.h>

#include "../IService.h"
#include "../ServiceRegistry.h"
#include "../eeprom/EEPROMService.h"

class BoilerplateService : public IService {
public:
    BoilerplateService(ServiceRegistry& registry, const char* tag = "BoilerplateService") 
        : registry(registry), TAG(tag), isReady(false) {}

    const char* getTag() const override {
        return TAG;
    }

    void start() override {
        Serial.print(TAG);
        Serial.println(" started.");

        EEPROMService* eeprom = registry.get<EEPROMService>("EEPROM");
        if (!eeprom) {
            Serial.println("EEPROMService not found in registry.");
            isReady = false;
            return;
        }
        
        eeprom->registerStaticEntry("score", sizeof(int));
        eeprom->registerStaticEntry("flag", sizeof(bool));

        int score = 1234;
        bool flag = true;

        eeprom->write("score", score);
        eeprom->write("flag", flag);

        int readScore;
        bool readFlag;

        eeprom->read("score", readScore);
        eeprom->read("flag", readFlag);

        Serial.print(TAG);
        Serial.print(" read score: ");
        Serial.println(readScore);
        Serial.print(TAG);
        Serial.print(" read flag: ");
        Serial.println(readFlag ? "true" : "false");

        eeprom->debugPrintUsage();

        isReady = true;
    }

    void update(unsigned long delta_ms) override {
        Serial.print(TAG);
        Serial.print(" update. Delta: ");
        Serial.print(delta_ms);
        Serial.println(" ms");
    }

    unsigned long cycleTimeMs() const override {
        return 10000;
    }

    bool ready() const override {
        return isReady;
    }

private:
    ServiceRegistry& registry;
    const char* TAG;
    bool isReady;
};

#endif
