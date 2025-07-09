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
//#include "service/boilerplate/BoilerplateService.h"
extern Scheduler scheduler;
//extern BoilerplateService service_boilerplate;
extern EEPROMService service_eeprom;
extern WiFiService service_wifi;
extern WebServerService service_webserver;

void startApp();
void updateApp();

#endif