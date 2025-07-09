#ifndef ISERVICE_H
#define ISERVICE_H
/// @brief these are to be registered in a ServiceRegistry
class IService {
public:
    virtual ~IService() = default;
    virtual void start() = 0;
    virtual bool ready() const = 0;
    virtual void update(unsigned long delta_ms) = 0;
    virtual unsigned long cycleTimeMs() const = 0;
    virtual const char* getTag() const = 0;
};
#endif