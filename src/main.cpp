#include "main.h"

static const char* TAG = "main";

Scheduler scheduler;
ServiceRegistry registry;

EEPROMService service_eeprom(registry, scheduler);
WiFiService service_wifi(registry, scheduler);
WebCookieService service_webcookie(registry, scheduler);
WebServerService service_webserver(registry, scheduler);
SDCardService service_sdcard(registry, scheduler);
SensorLoggingService service_sensorlog(registry, scheduler);

std::vector<IService*> coreServices = {
    &service_eeprom,
    &service_wifi,
    &service_sdcard,
    &service_webcookie,
    &service_webserver,
    &service_sensorlog
};

size_t currentServiceIndex = 0;
bool waitingForReady = false;

void startNextService() {
    if (currentServiceIndex >= coreServices.size()) {
        //schedule updates, exit setup
        for (auto service : coreServices) {
            unsigned long cycle = service->cycleTimeMs();
            if (cycle > 0) {
                scheduler.addTask([service]() {
                    service->update(service->cycleTimeMs());
                }, cycle, true);
            }
        }
        Serial.println("All services started.");
        waitingForReady = false;
        return;
    }

    IService* service = coreServices[currentServiceIndex];
    Serial.print(service->getTag());
    Serial.println(": Service starting...");
    service->start();
    waitingForReady = true;
}

void startApp() {
    Serial.begin(115200);
    delay(5);

    Serial.println("Registering services...");
    registry["EEPROM"] = &service_eeprom;
    registry["WIFI"] = &service_wifi;
    registry["WEBCOOKIE"] = &service_webcookie;
    registry["WEBSERVER"] = &service_webserver;
    registry["SDCARD"] = &service_sdcard;
    registry["SENSORLOG"] = &service_sensorlog;
    currentServiceIndex = 0;
    waitingForReady = false;

    startNextService();
}

void updateApp() {
    scheduler.update();
    if (waitingForReady) {
        IService* service = coreServices[currentServiceIndex];
        if (service->ready()) {
            Serial.print(service->getTag());
            Serial.println(": Service ready.");
            currentServiceIndex++;
            waitingForReady = false;
            startNextService();
        }
    }
}
