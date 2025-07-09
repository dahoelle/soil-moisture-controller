#ifndef SERVICE_REGISTRY_H
#define SERVICE_REGISTRY_H

#include <map>
#include <string>
#include <Arduino.h>

#include "IService.h"
///@brief creating servces ad-hoc is dangerous, they should be finite at start() and dependencies are to be start()ed in order 
class ServiceRegistry {
public:
    IService*& operator[](const std::string& name) {
        return services[name];
    }

    const IService* operator[](const std::string& name) const {
        auto it = services.find(name);
        if (it != services.end()) {
            return it->second;
        }
        return nullptr;
    }

    void registerService(const std::string& name, IService* service) {
        auto result = services.emplace(name, service);
        if (!result.second) {
            Serial.print("Warning: Service with name '");
            Serial.print(name.c_str());
            Serial.println("' already registered. Ignoring.");
        }
    }

    template<typename T>
    T* get(const std::string& name) const {
        auto it = services.find(name);
        if (it != services.end())
            return static_cast<T*>(it->second);
        return nullptr;
    }

private:
    std::map<std::string, IService*> services;
};

#endif
