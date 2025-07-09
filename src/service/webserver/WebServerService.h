#ifndef SERVICE_WEBSRV_H
#define SERVICE_WEBSRV_H

#include <AsyncTCP.h>
#include <ESPAsyncWebSrv.h>

#include "../IService.h"
#include "../ServiceRegistry.h"
#include "../eeprom/EEPROMService.h"
#include "../../scheduler/scheduler.h"
#include "index_html.h"

class WebServerService : public IService {
public:
    WebServerService(ServiceRegistry& registry, Scheduler& scheduler, const char* tag = "WebServerService")
        : scheduler(scheduler), server(80), registry(registry), TAG(tag), isReady(false) {}

    void start() override {
        // Captive portal redirects
        server.on("/connecttest.txt",       [](AsyncWebServerRequest* req) { req->redirect("/"); });
        server.on("/wpad.dat",              [](AsyncWebServerRequest* req) { req->send(404); });
        server.on("/generate_204",         [](AsyncWebServerRequest* req) { req->redirect("/"); });
        server.on("/redirect",              [](AsyncWebServerRequest* req) { req->redirect("/"); });
        server.on("/hotspot-detect.html",  [](AsyncWebServerRequest* req) { req->redirect("/"); });
        server.on("/canonical.html",       [](AsyncWebServerRequest* req) { req->redirect("/"); });
        server.on("/success.txt",           [](AsyncWebServerRequest* req) { req->send(200); });
        server.on("/ncsi.txt",              [](AsyncWebServerRequest* req) { req->redirect("/"); });

        // Root page
        server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
            req->send(200, "text/html", INDEX_HTML);
        });

        // /wifi-config/ PUT endpoint: saves ssid & pass to EEPROMService
        server.on("/wifi-config/", HTTP_PUT, [this](AsyncWebServerRequest* req) {
            if (!req->hasParam("SSID", true) || !req->hasParam("PASS", true)) {
                req->send(400, "application/json", "{\"error\":\"Missing ssid or passkey\"}");
                return;
            }

            String ssid = req->getParam("SSID", true)->value();
            String pass = req->getParam("PASS", true)->value();

            if (ssid.length() > 63 || pass.length() > 63) {
                req->send(400, "application/json", "{\"error\":\"ssid and passkey max length is 63\"}");
                return;
            }

            EEPROMService* eeprom = registry.get<EEPROMService>("EEPROM");
            if (!eeprom) {
                req->send(500, "application/json", "{\"error\":\"EEPROMService not available\"}");
                return;
            }

            eeprom->registerStaticEntry("SSID", 64);
            eeprom->registerStaticEntry("PASS", 64);

            char ssidBuf[64] = {0};
            char passBuf[64] = {0};

            ssid.toCharArray(ssidBuf, 64);
            pass.toCharArray(passBuf, 64);

            eeprom->write("SSID", ssidBuf);
            eeprom->write("PASS", passBuf);

            req->send(200, "application/json", "{\"status\":\"success\"}");

            // Schedule restart after response sent
            scheduler.addTask([]() -> bool {ESP.restart();}, 100, false);
        });

        // 404 handler
        server.onNotFound([](AsyncWebServerRequest* req) {
            Serial.printf("404 Not Found: %s\n", req->url().c_str());
            req->send(404, "text/plain", "Not found");
        });

        server.begin();
        isReady = true;
    }

    void update(unsigned long) override {} // noop

    const char* getTag() const override {
        return TAG;
    }

    unsigned long cycleTimeMs() const override {
        return 0;
    }

    bool ready() const override {
        return isReady;
    }

    AsyncWebServer& getServer() {
        return server;
    }

private:
    const char* TAG;
    ServiceRegistry& registry;
    Scheduler& scheduler;
    AsyncWebServer server;
    bool isReady;
};

#endif
