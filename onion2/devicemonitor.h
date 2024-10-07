// devicemonitor.h
#ifndef DEVICEMONITOR_H
#define DEVICEMONITOR_H

#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include "onion.h"

class DeviceMonitor {
public:
    DeviceMonitor();
    ~DeviceMonitor();

    void startMonitoring();
    void stopMonitoring();

private:
    std::vector<Onion> onionDevices;
    std::vector<std::string> detectDevices();
    
    std::atomic<bool> keepMonitoring;
    std::thread monitorThread;

    void monitorDevices();  // Continuously monitor device plugs/unplugs
    void addDevice(const std::string& deviceName);
    void removeDevice(const std::string& deviceName);
};

#endif // DEVICEMONITOR_H
