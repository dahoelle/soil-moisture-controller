#ifndef SERVICE_SDCARD_H
#define SERVICE_SDCARD_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

#include "../IService.h"
#include "../ServiceRegistry.h"

class SDCardService : public IService {
public:
    SDCardService(ServiceRegistry& registry, Scheduler& scheduler, const char* tag = "SDCardService")
        : registry(registry), scheduler(scheduler), TAG(tag), isReady(false) {}

    const char* getTag() const override {
        return TAG;
    }

    void start() override {
        Serial.print(TAG);
        Serial.println(": Initializing SD card with fixed SPI pins.");

        // Initialize SPI with fixed pins
        SPI.begin();//SCK, MOSI, MISO, CS);

        if (!SD.begin(CS)) {
            Serial.println(TAG);
            Serial.println(": SD card initialization failed!");
            isReady = false;
            return;
        }

        Serial.println(TAG);
        Serial.println(": SD card initialized.");

        // Example file write test
        File file = SD.open("/test.txt", FILE_WRITE);
        if (file) {
            file.println("SDCardService test write.");
            file.close();
            Serial.println(TAG);
            Serial.println(": Wrote to /test.txt");
        } else {
            Serial.println(TAG);
            Serial.println(": Failed to open /test.txt for writing");
        }

        if (SD.exists("/test.txt")) {
            Serial.println(TAG);
            Serial.println(": /test.txt exists on SD card");
        } else {
            Serial.println(TAG);
            Serial.println(": /test.txt does NOT exist");
        }

        isReady = true;
    }

    void update(unsigned long delta_ms) override {
        // noop
    }

    unsigned long cycleTimeMs() const override {
        return 0;
    }

    bool ready() const override {
        return isReady;
    }

private:
    ServiceRegistry& registry;
    Scheduler& scheduler;
    const char* TAG;
    bool isReady;
    /* config from https://www.reddit.com/r/esp32/comments/1awyih5/esp32c6_sdcard/?share_id=jdAB0gQoyfKai_9zwDq9X&utm_medium=ios_app&utm_name=ioscss&utm_source=share&utm_term=1
    static constexpr uint8_t CS = 18;//1;
    static constexpr uint8_t SCK = 21;//8;
    static constexpr uint8_t MOSI = 19;//10;
    static constexpr uint8_t MISO = 20;//11;
    */
   /*
   //VSPI defaults
   static constexpr uint8_t CS = 5;
    static constexpr uint8_t SCK = 18;
    static constexpr uint8_t MOSI = 23;
    static constexpr uint8_t MISO = 19;
    */
   /*ESP32C6-SD-Card.pdf inner
    static constexpr uint8_t CS = 18;
    static constexpr uint8_t MISO = 19;
    static constexpr uint8_t SCK = 20;
    static constexpr uint8_t MOSI = 21;
    */
   ///*ESP32C6-SD-Card.pdf outer
    static constexpr uint8_t CS = 18;
    static constexpr uint8_t MOSI = 19;
    static constexpr uint8_t MISO = 20;
    static constexpr uint8_t SCK = 21;
    
};

#endif
