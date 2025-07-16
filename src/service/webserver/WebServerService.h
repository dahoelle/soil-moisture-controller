#ifndef SERVICE_WEBSRV_H
#define SERVICE_WEBSRV_H

#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_NeoPixel.h>
#include "mbedtls/sha256.h"


#include "../IService.h"
#include "../ServiceRegistry.h"
#include "../eeprom/EEPROMService.h"
#include "../sdcard/SDCardService.h"
#include "../../scheduler/scheduler.h"
#include "html/index_html.h"
#include "html/editor_html.h"
#include "html/login_html.h"
#include "html/graph_html.h"
#include "html/filebrowser_html.h"

class WebServerService : public IService {
public:
    WebServerService(ServiceRegistry& registry, Scheduler& scheduler, const char* tag = "WebServerService") : scheduler(scheduler), server(80), registry(registry), TAG(tag), isReady(false) {}

    void start() override {
        strip.begin();
        strip.show();

        sd = registry.get<SDCardService>("SDCARD");

        // CAPTIVE PORTAL REDIRECTS
        server.on("/connecttest.txt", [](AsyncWebServerRequest* req) { req->redirect("/"); });
        server.on("/wpad.dat", [](AsyncWebServerRequest* req) { req->send(404); });
        server.on("/generate_204", [](AsyncWebServerRequest* req) { req->redirect("/"); });
        server.on("/redirect", [](AsyncWebServerRequest* req) { req->redirect("/"); });
        server.on("/hotspot-detect.html", [](AsyncWebServerRequest* req) { req->redirect("/"); });
        server.on("/canonical.html", [](AsyncWebServerRequest* req) { req->redirect("/"); });
        server.on("/success.txt", [](AsyncWebServerRequest* req) { req->send(200); });
        server.on("/ncsi.txt", [](AsyncWebServerRequest* req) { req->redirect("/"); });

        // STATIC PAGES
        server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {req->send(200, "text/html", INDEX_HTML);});
        server.on("/editor", HTTP_GET, [](AsyncWebServerRequest* req) { req->send(200, "text/html", EDITOR_HTML);});
        server.on("/login", HTTP_GET, [](AsyncWebServerRequest* req) {req->send(200, "text/html", LOGIN_HTML);});
        server.on("/graph", HTTP_GET, [](AsyncWebServerRequest* req) {req->send(200, "text/html", GRAPH_HTML);});
        server.on("/tree", HTTP_GET, [](AsyncWebServerRequest* req) {req->send(200, "text/html", FILEBROWSER_HTML);});
        //AUTH HANDLE
        server.on("/auth", HTTP_POST,
            [](AsyncWebServerRequest *req) { //CONTENT ASSERTION
                if (req->contentLength() == 0)
                    req->send(400, "application/json", "{\"error\":\"Empty body\"}");
                if (req->_tempObject != nullptr) 
                    return;
            },nullptr,
            [this](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t index, size_t total) { //CONTENT VALIDATION
                if (index != 0) 
                    return;
                DynamicJsonDocument doc(512);
                if (deserializeJson(doc, data, len)) {
                    req->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                    return;
                }

                String username = doc["name"] | "";
                String password = doc["pass"] | "";

                if (username == "" || password == "") {
                    req->send(400, "application/json", "{\"error\":\"Missing name or pass\"}");
                    return;
                }

                if (!sd || !sd->ready()) {
                    req->send(500, "application/json", "{\"error\":\"SD card not ready\"}");
                    return;
                }

                File configFile = sd->openFile("/config.json", FILE_READ);
                if (!configFile) {
                    req->send(500, "application/json", "{\"error\":\"Failed to read config.json\"}");
                    return;
                }

                DynamicJsonDocument cfg(2048);
                DeserializationError err = deserializeJson(cfg, configFile);
                configFile.close();
                if (err) {
                    req->send(500, "application/json", "{\"error\":\"Bad config.json\"}");
                    return;
                }

                bool found = false;
                String cookie;
                for (JsonObject user : cfg["administrators"].as<JsonArray>()) {
                    String name = user["name"] | "";
                    String salt = user["salt"] | "";
                    String hash = user["hash"] | "";

                    if (name != username) 
                        continue;

                    // SHA256(salt + password)
                    String toHash = salt + password;
                    uint8_t digest[32];
                    mbedtls_sha256_context ctx;
                    mbedtls_sha256_init(&ctx);
                    mbedtls_sha256_starts(&ctx, 0);  // 0 = SHA-256, 1 = SHA-224
                    mbedtls_sha256_update(&ctx, (const unsigned char*)toHash.c_str(), toHash.length());
                    mbedtls_sha256_finish(&ctx, digest);
                    mbedtls_sha256_free(&ctx);

                    char hex[65] = {0};
                    for (int i = 0; i < 32; ++i) 
                        sprintf(hex + i * 2, "%02x", digest[i]);

                    if (hash == String(hex)) {
                        found = true;
                        cookie = "auth=" + name + "; Max-Age=2592000; Path=/";
                        break;
                    }
                }

                if (found) {
                    AsyncWebServerResponse* res = req->beginResponse(200, "application/json", "{\"status\":\"ok\"}");
                    res->addHeader("Set-Cookie", cookie);
                    req->send(res);
                } else {
                    req->send(403, "application/json", "{\"error\":\"Auth failed\"}");
                }
            }
        );

        server.on("/wifi-config/", HTTP_PUT, 
            [this](AsyncWebServerRequest *req) { // AUTH REQUIRED
                if (!isAuthenticated(req)) {
                    req->send(403, "text/plain", "Forbidden");
                    return;
                }
            }, nullptr,
            [this](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t index, size_t total) {
                if (index != 0) 
                    return;
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
            }
        );
        server.on("/clear-logs/", HTTP_POST,
            [this](AsyncWebServerRequest* req) {
                if (!isAuthenticated(req)) {
                    req->send(403, "application/json", "{\"error\":\"Forbidden\"}");
                    return;
                }

                if (!sd || !sd->ready()) {
                    req->send(500, "application/json", "{\"error\":\"SD card not ready\"}");
                    return;
                }

                if (!sd->removeDirRecursive("/logs")) {
                    req->send(500, "application/json", "{\"error\":\"Failed to clear /logs directory\"}");
                    return;
                }
                req->send(200, "application/json", "{\"status\":\"logs cleared\"}");
            }
        );

        server.on("/restart/", HTTP_POST,
            [this](AsyncWebServerRequest* req) {
                if (!isAuthenticated(req)) {
                    req->send(403, "application/json", "{\"error\":\"Forbidden\"}");
                    return;
                }
                req->send(200, "application/json", "{\"status\":\"restarting\"}");
                scheduler.addTask([]() { ESP.restart(); }, 100, false);  // delay a bit to let response finish
            }
        );
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
            }
        );

        server.on("/api/tree/", HTTP_GET, 
            [this](AsyncWebServerRequest *request) {
                StaticJsonDocument<8192> doc;
                JsonObject root = doc.to<JsonObject>();
                sd->buildFileTree(root);

                String output;
                serializeJson(doc, output);
                request->send(200, "application/json", output);
            }
        );

        server.on("/api/file/*", HTTP_GET,
            [this](AsyncWebServerRequest* req) {
                if (!isAuthenticated(req)) {
                    req->send(403, "text/plain", "Forbidden");
                    return;
                }
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
            }
        );

        server.on("/api/file/*", HTTP_PUT, 
            [this](AsyncWebServerRequest *req) { // AUTH REQUIRED
                if (!isAuthenticated(req)) {
                    req->send(403, "text/plain", "Forbidden");
                    return;
                }
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
            }
        );
        server.on("/api/storage", HTTP_GET, 
            [this](AsyncWebServerRequest *request) {
                DynamicJsonDocument doc(256);
                doc["total"] = sd->getTotalSpace();
                doc["used"] = sd->getUsedSpace();
                doc["free"] = sd->getFreeSpace();
                String json;
                serializeJson(doc, json);
                request->send(200, "application/json", json);
            }
        );

        // 404
        server.onNotFound(
            [](AsyncWebServerRequest* req) {
                Serial.printf("404 Not Found: %s\n", req->url().c_str());
                req->send(404, "text/plain", "Not found");
            }
        );

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
    WiFiService* wifi;

    bool isAuthenticated(AsyncWebServerRequest* req) {
        if (!req->hasHeader("Cookie")) 
            return false;
        String cookie = req->header("Cookie");
        return cookie.indexOf("auth=admin") != -1;
    }
};

#endif
