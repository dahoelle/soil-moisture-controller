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

        SPI.begin();

        if (!SD.begin(CS)) {
            Serial.println(TAG);
            Serial.println(": SD card initialization failed!");
            isReady = false;
            return;
        }

        Serial.println(TAG);
        Serial.println(": SD card initialized.");




        if (!SD.exists("/config.json")) {
            Serial.println("Creating default config.json on SD card...");

            File file = SD.open("/config.json", FILE_WRITE);
            if (!file) {
                Serial.println("Failed to create config.json!");
                return;
            }

            // Write a simple default JSON object — adjust as you want
            const char* defaultJson = R"({
            "ssid": "default_ssid",
            "pass": "default_pass"
            })";

            file.print(defaultJson);
            file.close();

            Serial.println("config.json created successfully.");
        } else {
            Serial.println("config.json already exists.");
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

    //==== API ======
    bool fileExists(const String& path) {
        return SD.exists(path);
    }

    bool removeFile(const String& path) {
        return SD.remove(path);
    }

    bool createFile(const String& path, const String& content = "") {
        File file = SD.open(path, FILE_WRITE);
        if (!file) return false;
        if (content.length()) file.print(content);
        file.close();
        return true;
    }
    File openFile(const String& path, const char* mode) {
        return SD.open(path, mode);
    }
    bool readFile(const String& path, String& outContent) {
        File file = SD.open(path, FILE_READ);
        if (!file) return false;
        outContent = "";
        while (file.available()) outContent += (char)file.read();
        file.close();
        return true;
    }

    bool writeFile(const String& path, const String& content) {
        File file = SD.open(path, FILE_WRITE);
        if (!file) return false;
        file.print(content);
        file.close();
        return true;
    }

    bool appendFile(const String& path, const String& content) {
        File file = SD.open(path, FILE_APPEND);
        if (!file) return false;
        file.print(content);
        file.close();
        return true;
    }

    bool createDir(const String& path) {
        return SD.mkdir(path);
    }

    bool removeDir(const String& path) {
        return SD.rmdir(path);
    }

    bool listDir(const String& path, String& outList, int depth = 1) {
        File root = SD.open(path);
        if (!root || !root.isDirectory()) return false;

        File file = root.openNextFile();
        while (file) {
            outList += file.name();
            outList += file.isDirectory() ? "/\n" : "\n";
            file = root.openNextFile();
        }
        return true;
    }

private:
    ServiceRegistry& registry;
    Scheduler& scheduler;
    const char* TAG;
    bool isReady;

    static constexpr uint8_t CS = 18;
    static constexpr uint8_t MOSI = 19;
    static constexpr uint8_t MISO = 20;
    static constexpr uint8_t SCK = 21;
};

#endif
