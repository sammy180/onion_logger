// onion.cpp
#include "onion.h"
#include <chrono>

Onion::Onion(const std::string& deviceName) : deviceName(deviceName), keepRunning(true) {
    std::cout << "Onion device " << deviceName << " initialized." << std::endl;
    openPort();
    startDataThread();
}

Onion::~Onion() {
    stopDataThread();
}

Onion::Onion(Onion&& other) noexcept
    : deviceName(std::move(other.deviceName)), keepRunning(other.keepRunning.load()), dataThread(std::move(other.dataThread)) {
    other.keepRunning = false;
}

Onion& Onion::operator=(Onion&& other) noexcept {
    if (this != &other) {
        stopDataThread();  // Stop current thread if running

        deviceName = std::move(other.deviceName);
        keepRunning = other.keepRunning.load();
        dataThread = std::move(other.dataThread);
        other.keepRunning = false;
    }
    return *this;
}

void Onion::openPort() {
    std::cout << "Opening port for " << deviceName << std::endl;
}

void Onion::startDataThread() {
    dataThread = std::thread(&Onion::recordData, this);
}

void Onion::stopDataThread() {
    keepRunning = false;
    if (dataThread.joinable()) {
        dataThread.join();
    }
}

void Onion::recordData() {
    while (keepRunning) {
        std::cout << "Recording data from " << deviceName << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));  // Simulate data delay
    }
    std::cout << "Stopped recording data from " << deviceName << std::endl;
}

std::string Onion::getDeviceName() const {
    return deviceName;
}
