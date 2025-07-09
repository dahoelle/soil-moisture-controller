#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <vector>
#include <functional>
#include <Arduino.h>

class Scheduler {
public:
    Scheduler() {}

    // Add a task: callback returns void; repeat controls if task runs repeatedly
    void addTask(std::function<void()> cb, unsigned long interval_ms, bool repeat = true) {
        unsigned long now = millis();
        tasks.push_back({cb, interval_ms, now, repeat});
    }

    void update() {
        unsigned long now = millis();
        for (auto it = tasks.begin(); it != tasks.end(); ) {
            unsigned long elapsed = now - it->last_run;
            if (elapsed >= it->interval_ms) {
                it->callback();
                if (it->repeat) {
                    it->last_run += it->interval_ms;
                    ++it;
                } else {
                    it = tasks.erase(it);
                }
            } else {
                ++it;
            }
        }
    }

private:
    struct SchedulerTask {
        std::function<void()> callback;
        unsigned long interval_ms;
        unsigned long last_run;
        bool repeat;
    };

    std::vector<SchedulerTask> tasks;
};

#endif
