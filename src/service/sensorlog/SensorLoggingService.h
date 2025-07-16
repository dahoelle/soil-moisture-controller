#ifndef SERVICE_SENSORLOGGING_H
#define SERVICE_SENSORLOGGING_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "../IService.h"
#include "../ServiceRegistry.h"
#include "../sdcard/SDCardService.h"
#include "../wifi/WiFiService.h"

#define MAX_FILE_SIZE (20 * 1024 * 1024)  // 20MB

constexpr uint8_t PIN_ADC4 = 4;  // ADC1_CH4
constexpr uint8_t PIN_ADC5 = 5;  // ADC1_CH5
constexpr uint8_t PIN_ADC6 = 6;  // ADC1_CH6

class SensorLoggingService : public IService {
public:
    SensorLoggingService(ServiceRegistry& registry, Scheduler& scheduler, const char* tag = "SensorLoggingService")
        : registry(registry), scheduler(scheduler), TAG(tag), isReady(false) {}

    void start() override {
        sd = registry.get<SDCardService>("SDCARD");
        wifi = registry.get<WiFiService>("WIFI");

        if (!sd || !sd->ready() || !wifi || !wifi->ready()) {
            Serial.println("SensorLoggingService: Required services not available.");
            return;
        }

        ensureLogDir();
        isReady = true;
        Serial.println("SensorLoggingService: Started.");
    }

    void update(unsigned long) override {
        if (!isReady) return;

        switch (state) {
            case READ_ADC4:
                valueADC4 = analogRead(PIN_ADC4);
                state = READ_ADC5;
                break;

            case READ_ADC5:
                valueADC5 = analogRead(PIN_ADC5);
                state = READ_ADC6;
                break;

            case READ_ADC6:
                valueADC6 = analogRead(PIN_ADC6);
                state = LOG_ALL;
                break;

            case LOG_ALL:
                logSensors();
                state = READ_ADC4;
                break;
        }
    }

    const char* getTag() const override { return TAG; }
    bool ready() const override { return isReady; }
    unsigned long cycleTimeMs() const override { return 333; }

private:
    const char* TAG;
    ServiceRegistry& registry;
    Scheduler& scheduler;
    SDCardService* sd = nullptr;
    WiFiService* wifi = nullptr;
    bool isReady;

    File currentFile;
    String currentPath;

    enum State { READ_ADC4, READ_ADC5, READ_ADC6, LOG_ALL };
    State state = READ_ADC4;

    int valueADC4 = 0;
    int valueADC5 = 0;
    int valueADC6 = 0;

    void ensureLogDir() {
        if (!sd->fileExists("/logs")) {
            sd->createDir("/logs");
        }
    }

    String getLogFilePath(time_t now) {
        struct tm* tmInfo = localtime(&now);

        char timeStr[32];
        strftime(timeStr, sizeof(timeStr), "/logs/%Y%m%d_%H", tmInfo);

        int index = 0;
        String base = String(timeStr);

        while (true) {
            String path = base;
            if (index > 0) {
                path += "_" + String(index);
            }
            path += ".json";

            if (!sd->fileExists(path) || sd->openFile(path, FILE_READ).size() < MAX_FILE_SIZE) {
                return path;
            }
            index++;
        }
    }

    void rotateFile(const String& newPath) {
        if (currentFile) {
            currentFile.close();
        }

        currentFile = sd->openFile(newPath, FILE_APPEND);
        if (!currentFile) {
            Serial.printf("SensorLoggingService: Failed to open log file %s\n", newPath.c_str());
            return;
        }

        currentPath = newPath;
        Serial.printf("SensorLoggingService: Logging to %s\n", newPath.c_str());
    }

    void logSensors() {
        time_t now = wifi->getUnixTime();
        String filePath = getLogFilePath(now);

        if (!currentFile || currentPath != filePath || currentFile.size() >= MAX_FILE_SIZE) {
            rotateFile(filePath);
        }

        DynamicJsonDocument doc(512);
        doc["timestamp"] = now;
        doc["ADC4"] = valueADC4;
        doc["ADC5"] = valueADC5;
        doc["ADC6"] = valueADC6;

        serializeJson(doc, currentFile);
        currentFile.print('\n');
        currentFile.flush();
    }
};

#endif
