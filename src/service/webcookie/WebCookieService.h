#ifndef SERVICE_WEBCOOKIE_H
#define SERVICE_WEBCOOKIE_H

#include <map>
#include <ArduinoJson.h>
#include <FS.h>
#include <SD.h>
#include <Arduino.h>
#include "../IService.h"
#include "../ServiceRegistry.h"
#include "../sdcard/SDCardService.h"
#include "../wifi/WiFiService.h"

struct CookieInfo {
    String name;
    time_t expireDate;
    time_t deletionDate;
    bool expired;
};

class WebCookieService : public IService {
public:
    WebCookieService(ServiceRegistry& registry, Scheduler& scheduler, const char* tag= "SensorLoggingService") : registry(registry), scheduler(scheduler), TAG(tag), isReady(false) {}

    void start() override {
        sd = registry.get<SDCardService>("SDCARD");
        wifi = registry.get<WiFiService>("WIFI");

        if (!sd || !sd->ready() || !wifi || !wifi->ready()) {
            Serial.println("WebCookieService: Required services not available.");
            return;
        }

        loadFromFile();
        isReady = true;
    }

    void update(unsigned long) override {
        time_t now = wifi->getUnixTime();
        bool dirty = false;
        for (auto it = cookies.begin(); it != cookies.end(); ) {
            CookieInfo& info = it->second;
            if (!info.expired && now >= info.expireDate) {
                info.expired = true;
                dirty = true;
            }
            if (info.expired && now >= info.deletionDate) {
                it = cookies.erase(it);
                dirty = true;
            } else {
                ++it;
            }
        }
        if (dirty) 
            saveToFile();
    }

    const char* getTag() const override {
        return TAG;
    }

    bool ready() const override {
        return isReady;
    }

    unsigned long cycleTimeMs() const override {
        return 10000; // 10s polling interval
    }

    void setCookie(const String& cookieStr, const String& name, unsigned long expireDelta, unsigned long deletionDelta) {
        time_t now = wifi->getUnixTime();
        cookies[cookieStr] = CookieInfo{
            name,
            now + expireDelta,
            now + deletionDelta,
            false
        };
        saveToFile();
    }

    bool has(const String& cookieStr) const {
        return cookies.find(cookieStr) != cookies.end();
    }

    std::pair<String, bool> operator[](const String& cookieStr) const {
        auto it = cookies.find(cookieStr);
        if (it != cookies.end())
            return { it->second.name, it->second.expired };
        return { "", true };
    }

private:
    const char* TAG;
    ServiceRegistry& registry;
    Scheduler& scheduler;
    SDCardService* sd;
    WiFiService* wifi;
    bool isReady;
    std::map<String, CookieInfo> cookies;

    void loadFromFile() {
        File file = sd->openFile("/cookies.json", FILE_READ);
        if (!file) return;

        DynamicJsonDocument doc(8192);
        if (deserializeJson(doc, file)) {
            file.close();
            Serial.println("WebCookieService: Failed to parse cookies.json");
            return;
        }
        file.close();

        for (JsonPair kv : doc.as<JsonObject>()) {
            CookieInfo info;
            info.name = kv.value()["name"].as<String>();
            info.expireDate = kv.value()["expireDate"] | 0;
            info.deletionDate = kv.value()["deletionDate"] | 0;
            info.expired = kv.value()["expired"] | false;
            cookies[kv.key().c_str()] = info;
        }
    }

    void saveToFile() {
        DynamicJsonDocument doc(8192);
        for (const auto& kv : cookies) {
            JsonObject obj = doc.createNestedObject(kv.first);
            obj["name"] = kv.second.name;
            obj["expireDate"] = kv.second.expireDate;
            obj["deletionDate"] = kv.second.deletionDate;
            obj["expired"] = kv.second.expired;
        }

        File file = sd->openFile("/cookies.json", "w");
        if (!file) {
            Serial.println("WebCookieService: Failed to write cookies.json");
            return;
        }
        serializeJsonPretty(doc, file);
        file.close();
    }
};

#endif
