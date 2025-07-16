#ifndef SERVICE_WIFI_H
#define SERVICE_WIFI_H

#include <WiFi.h>
#include <DNSServer.h>
#include <Arduino.h>
#include <time.h>

#include "../IService.h"
#include "../ServiceRegistry.h"
#include "../eeprom/EEPROMService.h"
#include "../../scheduler/scheduler.h"

class WiFiService : public IService {
public:
    WiFiService(ServiceRegistry& registry, Scheduler& scheduler, const char* tag = "WiFiService") : registry(registry), scheduler(scheduler), TAG(tag), isReady(false), apMode(false) {}

    void start() override {
        EEPROMService* eeprom = registry.get<EEPROMService>("EEPROM");
        if (!eeprom) {
            Serial.println("WiFiService: EEPROMService not available.");
            isReady = false;
            return;
        }

        eeprom->registerStaticEntry("SSID", 64);
        eeprom->registerStaticEntry("PASS", 64);

        eeprom->read("SSID", ssid);
        eeprom->read("PASS", pass);

        ssid[63] = '\0';
        pass[63] = '\0';

        for (int i = 0; i < strlen(ssid); ++i) {
            if (!isPrintable(ssid[i])) {
                retriesLeft = 0;
                break;
            }
        }

        WiFi.disconnect(true);
        WiFi.setHostname("esp32-device");
        delay(50);

        if (ssid[0] == '\0') {
            startAccessPoint();
            return;
        }

        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, pass);
        WiFi.setAutoReconnect(true);

        Serial.print("WiFiService: Connecting to SSID: ");
        Serial.println(ssid);

        for (int i = 0; i < retriesLeft; ++i) {
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("WiFiService: Connected!");
                Serial.print("Local IP: ");
                Serial.println(WiFi.localIP());

                // NTP setup
                configTime(3600, 0, "pool.ntp.org", "time.nist.gov");

                Serial.print("WiFiService: Syncing time via NTP");
                time_t now = time(nullptr);
                int retry = 0;
                while (now < 8 * 3600 * 2 && retry++ < 20) {
                    delay(500);
                    Serial.print(".");
                    now = time(nullptr);
                }
                Serial.println();

                if (now < 8 * 3600 * 2) {
                    Serial.println("WiFiService: NTP sync failed. Time not set.");
                } else {
                    Serial.print("WiFiService: Time synced: ");
                    Serial.println(ctime(&now));
                }

                isReady = true;
                apMode = false;
                return;
            }
            Serial.print(".");
            delay(1000);
        }

        Serial.println("\nWiFiService: Failed to connect.");
        clearCredentials(eeprom);
        delay(1000);
        ESP.restart();
    }

    void update(unsigned long) override {
        if (apMode)
            dnsServer.processNextRequest();
    }

    unsigned long cycleTimeMs() const override {
        return 0;
    }

    bool ready() const override {
        return isReady;
    }

    const char* getTag() const override {
        return "WiFiService";
    }

    bool getTime(struct tm* timeInfo) const {
        return getLocalTime(timeInfo);
    }

    time_t getUnixTime() const {
        return time(nullptr);
    }

    bool isTimeSynced() const {
        return time(nullptr) > 8 * 3600 * 2;
    }


private:
    ServiceRegistry& registry;
    Scheduler& scheduler;
    const char* TAG;

    DNSServer dnsServer;
    char ssid[64] = {0};
    char pass[64] = {0};
    uint8_t retriesLeft = 100;
    bool isReady;
    bool apMode;

    void startAccessPoint() {
        Serial.println("WiFiService: Starting in AP mode.");

        WiFi.mode(WIFI_AP);
        IPAddress apIP(192, 168, 0, 1);
        IPAddress gateway(192, 168, 0, 1);
        IPAddress subnet(255, 255, 255, 0);

        WiFi.softAPConfig(apIP, gateway, subnet);
        WiFi.softAP("ESP32-AP");

        dnsServer.setTTL(3600);
        dnsServer.start(53, "*", apIP);

        scheduler.addTask([this]() {
            dnsServer.processNextRequest();
        }, 500, true);

        isReady = true;
    }

    void clearCredentials(EEPROMService* eeprom) {
        const char empty[64] = {0};
        eeprom->write("SSID", empty);
        eeprom->write("PASS", empty);
    }
};

#endif
