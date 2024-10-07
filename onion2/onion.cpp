// onion.cpp
#include "onion.h"
#include <chrono>
#include <fcntl.h>  // For opening the port
#include <unistd.h>
#include <termios.h>
#include <sqlite3.h>  // For database interaction

extern void read_device(const std::string& device_path, sqlite3* db, const std::string& device_name);


Onion::Onion(const std::string& deviceName, const std::string& devicePath)
    : deviceName(deviceName), devicePath(devicePath), keepRunning(true) {
    std::cout << "Onion device " << deviceName << " initialized." << std::endl;
    startDataThread();
}

Onion::~Onion() {
    stopDataThread();
}

Onion::Onion(Onion&& other) noexcept
    : deviceName(std::move(other.deviceName)), devicePath(std::move(other.devicePath)),
      keepRunning(other.keepRunning.load()), dataThread(std::move(other.dataThread)) {
    other.keepRunning = false;
}

Onion& Onion::operator=(Onion&& other) noexcept {
    if (this != &other) {
        stopDataThread();  // Stop current thread if running

        deviceName = std::move(other.deviceName);
        devicePath = std::move(other.devicePath);
        keepRunning = other.keepRunning.load();
        dataThread = std::move(other.dataThread);
        other.keepRunning = false;
    }
    return *this;
}

void Onion::startDataThread() {
    // Start the thread that will call read_device for data reading
    dataThread = std::thread(&Onion::readData, this);
}

void Onion::stopDataThread() {
    keepRunning = false;
    if (dataThread.joinable()) {
        dataThread.join();  // Join the thread to stop safely
    }
}

void Onion::readData() {
    sqlite3* db = nullptr;
    // Open the SQLite database (for demonstration purposes, the database file path could be adjusted)
    int rc = sqlite3_open("sensor_data.db", &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    while (keepRunning) {
        // Call the read_device function, which opens the port and reads incoming data
        read_device(devicePath, db, deviceName);
        std::this_thread::sleep_for(std::chrono::seconds(1));  // Adjust delay based on data rate
    }

    // Close the database after reading
    sqlite3_close(db);
}

std::string Onion::getDeviceName() const {
    return deviceName;
}
