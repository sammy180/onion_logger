// devicemonitor.cpp
#include "devicemonitor.h"
#include <iostream>
#include <chrono>
#include <algorithm>

DeviceMonitor::DeviceMonitor() : keepMonitoring(true) {
}

DeviceMonitor::~DeviceMonitor() {
    stopMonitoring();  // Ensure the thread stops before destruction
}

void DeviceMonitor::startMonitoring() {
    monitorThread = std::thread(&DeviceMonitor::monitorDevices, this);
}

void DeviceMonitor::stopMonitoring() {
    keepMonitoring = false;
    if (monitorThread.joinable()) {
        monitorThread.join();
    }
}

void DeviceMonitor::monitorDevices() {
    while (keepMonitoring) {
        std::vector<std::string> currentDevices = detectDevices();

        // Add new devices
        for (const auto& device : currentDevices) {
            if (std::none_of(onionDevices.begin(), onionDevices.end(), [&](const Onion& o) {
                return o.getDeviceName() == device;  // Check if device is already added
            })) {
                addDevice(device);
            }
        }

        // Remove unplugged devices
        onionDevices.erase(
            std::remove_if(onionDevices.begin(), onionDevices.end(), [&](const Onion& o) {
                if (std::find(currentDevices.begin(), currentDevices.end(), o.getDeviceName()) == currentDevices.end()) {
                    removeDevice(o.getDeviceName());  // Remove the device if it's no longer detected
                    return true;
                }
                return false;
            }),
            onionDevices.end()
        );

        std::this_thread::sleep_for(std::chrono::seconds(5));  // Sleep for a bit before rechecking
    }
}

void DeviceMonitor::addDevice(const std::string& deviceName) {
    if (deviceName == "Onion1" || deviceName == "Onion2" || deviceName == "Onion3" || deviceName == "Onion4") {
        std::cout << "Detected: " << deviceName << std::endl;
        onionDevices.emplace_back(deviceName);  // Add device and start monitoring data
    }
}

void DeviceMonitor::removeDevice(const std::string& deviceName) {
    std::cout << "Device " << deviceName << " unplugged." << std::endl;
}

std::vector<std::string> DeviceMonitor::detectDevices() {
    // Placeholder function for device detection
    return {"Onion1", "Onion2"};  // Example devices, this would actually detect connected devices
}
