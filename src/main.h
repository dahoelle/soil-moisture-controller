#ifndef MAIN_H
#define MAIN_H

#include <vector>

#include <Arduino.h>

#include "scheduler/scheduler.h"
#include "service/IService.h"
#include "service/ServiceRegistry.h"

#include "service/ServiceRegistry.h"
#include "service/eeprom/EEPROMService.h"
#include "service/wifi/WiFiService.h"
#include "service/webserver/WebServerService.h"
#include "service/sdcard/SDCardService.h"

extern Scheduler scheduler;

extern EEPROMService service_eeprom;
extern WiFiService service_wifi;
extern WebServerService service_webserver;
extern SDCardService service_sdcard;

void startApp();
void updateApp();

#endif