#ifndef SERVICE_WIFI_H
#define SERVICE_WIFI_H

#include <WiFi.h>
#include <DNSServer.h>
#include <Arduino.h>

#include "../IService.h"
#include "../ServiceRegistry.h"
#include "../eeprom/EEPROMService.h"
#include "../../scheduler/scheduler.h"

class WiFiService : public IService {
public:
    WiFiService(ServiceRegistry& registry, Scheduler& scheduler, const char* tag = "WiFiService") 
        : registry(registry), scheduler(scheduler), TAG(tag), isReady(false), apMode(false) {}

    const char* getTag() const override {
        return TAG;
    }

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
        if (apMode) {
            dnsServer.processNextRequest();
        }
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

    DNSServer dnsServer;
    char ssid[64] = {0};
    char pass[64] = {0};
    uint8_t retriesLeft = 10;
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
