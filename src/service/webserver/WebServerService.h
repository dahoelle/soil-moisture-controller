#ifndef SERVICE_WEBSRV_H
#define SERVICE_WEBSRV_H

#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_NeoPixel.h>

#include "../IService.h"
#include "../ServiceRegistry.h"
#include "../eeprom/EEPROMService.h"
#include "../sdcard/SDCardService.h"
#include "../../scheduler/scheduler.h"
#include "index_html.h"
#include "editor_html.h"

class WebServerService : public IService {
public:
    WebServerService(ServiceRegistry& registry, Scheduler& scheduler, const char* tag = "WebServerService")
        : scheduler(scheduler), server(80), registry(registry), TAG(tag), isReady(false) {}

    void start() override {
        strip.begin();
        strip.show();

        sd = registry.get<SDCardService>("SDCARD");

        // Redirects for captive portal detection and nonsense requests
        server.on("/connecttest.txt", [](AsyncWebServerRequest* req) { req->redirect("/"); });
        server.on("/wpad.dat", [](AsyncWebServerRequest* req) { req->send(404); });
        server.on("/generate_204", [](AsyncWebServerRequest* req) { req->redirect("/"); });
        server.on("/redirect", [](AsyncWebServerRequest* req) { req->redirect("/"); });
        server.on("/hotspot-detect.html", [](AsyncWebServerRequest* req) { req->redirect("/"); });
        server.on("/canonical.html", [](AsyncWebServerRequest* req) { req->redirect("/"); });
        server.on("/success.txt", [](AsyncWebServerRequest* req) { req->send(200); });
        server.on("/ncsi.txt", [](AsyncWebServerRequest* req) { req->redirect("/"); });

        // Serve main pages
        server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
            req->send(200, "text/html", INDEX_HTML);
        });
        server.on("/editor", HTTP_GET, [](AsyncWebServerRequest* req) {
            req->send(200, "text/html", EDITOR_HTML);
        });

        // WiFi config endpoint (PUT with JSON body)
        server.on("/wifi-config/", HTTP_PUT, [](AsyncWebServerRequest *req) {}, nullptr,
            [this](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t index, size_t total) {
                if (index != 0) return; // only process once when index == 0
                DynamicJsonDocument doc(256);
                DeserializationError error = deserializeJson(doc, data, len);

                if (error) {
                    req->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                    return;
                }

                String ssid = doc["ssid"] | "";
                String pass = doc["pass"] | "";

                if (ssid.length() == 0 || pass.length() == 0) {
                    req->send(400, "application/json", "{\"error\":\"Missing ssid or pass\"}");
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

                Serial.printf("Got WiFi config: ssid=%s, pass=%s\n", ssid.c_str(), pass.c_str());

                eeprom->write("SSID", ssidBuf);
                eeprom->write("PASS", passBuf);

                req->send(200, "application/json", "{\"status\":\"success\"}");

                scheduler.addTask([]() { ESP.restart(); }, 100, false);
            });

        // LED control endpoint (POST with JSON body)
        server.on("/led/", HTTP_POST, [](AsyncWebServerRequest *req) {}, nullptr,
            [this](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t index, size_t total) {
                if (index != 0) return; // only process once

                DynamicJsonDocument doc(128);
                DeserializationError err = deserializeJson(doc, data, len);

                if (err) {
                    req->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                    return;
                }

                if (!doc.containsKey("r") || !doc.containsKey("g") || !doc.containsKey("b")) {
                    strip.setPixelColor(0, strip.Color(0, 0, 0));
                    strip.show();
                    req->send(200, "application/json", "{\"status\":\"off\",\"r\":0,\"g\":0,\"b\":0}");
                    return;
                }

                uint8_t r = doc["r"];
                uint8_t g = doc["g"];
                uint8_t b = doc["b"];

                strip.setPixelColor(0, strip.Color(r, g, b));
                strip.show();

                Serial.printf("Set LED to R=%d G=%d B=%d\n", r, g, b);

                DynamicJsonDocument res(128);
                res["status"] = "on";
                res["r"] = r;
                res["g"] = g;
                res["b"] = b;

                String response;
                serializeJson(res, response);
                req->send(200, "application/json", response);
            });


        server.on("/api/file/*", HTTP_GET, [this](AsyncWebServerRequest* req) {
            String path = req->url(); 
            String wildcard = path.substring(strlen("/api/file")); 
            
            if (!sd || !sd->ready()) {
                req->send(500, "application/json", "{\"error\":\"SD card not ready\"}");
                return;
            }

            if (!wildcard.startsWith("/")) wildcard = "/" + wildcard;

            if (!sd->fileExists(wildcard)) {
                req->send(404, "application/json", "{\"error\":\"File not found\"}");
                return;
            }

            File file = sd->openFile(wildcard, FILE_READ);
            if (!file || file.isDirectory()) {
                req->send(500, "application/json", "{\"error\":\"Failed to open file\"}");
                return;
            }

            req->send(req->beginResponse(file, wildcard, "application/octet-stream", false));
        });

        server.on("/api/file/*", HTTP_PUT, [this](AsyncWebServerRequest *req) {
            if (req->contentLength() == 0) {
                req->send(400, "application/json", "{\"error\":\"Empty body\"}");
                return;
            }
        }, nullptr,
        [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index != 0)
                return;  // handle only once

            if (!sd || !sd->ready()) {
                req->send(500, "application/json", "{\"error\":\"SD card not ready\"}");
                return;
            }

            String fullUrl = req->url();  // e.g. "/api/file/path/to/file.txt"
            String filepath = fullUrl.substring(strlen("/api/file")); // "/path/to/file.txt"
            if (!filepath.startsWith("/")) filepath = "/" + filepath;

            // JSON payload expected to be: { "content": "the file contents here" }
            DynamicJsonDocument doc(2048);
            auto err = deserializeJson(doc, data, len);
            if (err) {
                req->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }

            String content = doc["content"] | "";
            if (content.length() == 0) {
                req->send(400, "application/json", "{\"error\":\"Missing content\"}");
                return;
            }

            if (filepath.indexOf("..") != -1) {
                req->send(400, "application/json", "{\"error\":\"Invalid file path\"}");
                return;
            }

            File file = sd->openFile(filepath, "w");
            if (!file) {
                req->send(500, "application/json", "{\"error\":\"Failed to open file for writing\"}");
                return;
            }

            size_t written = file.print(content);
            file.close();

            if (written != content.length()) {
                req->send(500, "application/json", "{\"error\":\"Failed to write full content\"}");
                return;
            }

            req->send(200, "application/json", "{\"status\":\"saved\"}");
        });



        // Not found handler for everything else
        server.onNotFound([](AsyncWebServerRequest* req) {
            Serial.printf("404 Not Found: %s\n", req->url().c_str());
            req->send(404, "text/plain", "Not found");
        });

        server.begin();
        isReady = true;
    }

    void update(unsigned long) override {}

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
    Adafruit_NeoPixel strip{1, 8, NEO_GRB + NEO_KHZ800};

    SDCardService* sd;
};

#endif
