// main.cpp
#include "devicemonitor.h"

int main() {
    DeviceMonitor monitor;
    monitor.startMonitoring();  // Start continuous device monitoring

    std::this_thread::sleep_for(std::chrono::seconds(30));  // Let it run for 30 seconds
    monitor.stopMonitoring();  // Stop the monitoring thread gracefully

    return 0;
}
