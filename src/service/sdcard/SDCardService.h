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
    bool removeDirRecursive(const String& path) {
        if (!SD.exists(path)) return false;

        File dir = SD.open(path);
        if (!dir || !dir.isDirectory()) return false;

        File file = dir.openNextFile();
        while (file) {
            String fname = file.name();
            String fullPath = path;
            if (!fullPath.endsWith("/")) fullPath += "/";
            fullPath += fname;

            if (file.isDirectory()) {
                if (!removeDirRecursive(fullPath)) {
                    file.close();
                    dir.close();
                    return false;
                }
            } else {
                if (!SD.remove(fullPath)) {
                    file.close();
                    dir.close();
                    return false;
                }
            }
            file.close();
            file = dir.openNextFile();
        }
        dir.close();

        return SD.rmdir(path);
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
    
    void buildFileTree(JsonObject &out) {
        File root = SD.open("/");
        if (!root || !root.isDirectory()) {
            out["error"] = "Failed to open SD root";
            return;
        }
        buildTreeRecursive(root, out);
        root.close();
    }

    void buildTreeRecursive(File dir, JsonObject &node) {
        node["name"] = String(dir.name());
        node["type"] = "directory";
        JsonArray children = node.createNestedArray("children");

        File entry = dir.openNextFile();
        while (entry) {
            JsonObject child = children.createNestedObject();
            if (entry.isDirectory()) {
            buildTreeRecursive(entry, child);
            } else {
            child["name"] = String(entry.name());
            child["type"] = "file";
            }
            entry.close();
            entry = dir.openNextFile();
        }
    }

    uint64_t getUsedSpace() {
        File root = SD.open("/");
        if (!root) return 0;
        uint64_t used = calculateUsedSpace(root);
        root.close();
        return used;
    }
    uint64_t calculateUsedSpace(File dir) {
        uint64_t total = 0;
        File entry = dir.openNextFile();
        while (entry) {
            if (entry.isDirectory()) {
                total += calculateUsedSpace(entry);  // Recurse
            } else {
                total += entry.size();  // File size
            }
            entry.close();
            entry = dir.openNextFile();
        }
        return total;
    }

    uint64_t getTotalSpace() {
        return 24ULL * 1024 * 1024 * 1024;  // 2GB in bytes
    }
    uint64_t getFreeSpace() {
        return getTotalSpace() - getUsedSpace();
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
