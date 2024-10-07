// onion.h
#ifndef ONION_H
#define ONION_H

#include <string>
#include <iostream>
#include <thread>
#include <atomic>

class Onion {
public:
    Onion(const std::string& deviceName, const std::string& devicePath);  // Constructor with device path
    ~Onion();  // Destructor
    
    Onion(const Onion&) = delete;  // Disable copy constructor
    Onion& operator=(const Onion&) = delete;  // Disable copy assignment

    Onion(Onion&& other) noexcept;  // Move constructor
    Onion& operator=(Onion&& other) noexcept;  // Move assignment

    void startDataThread();  // Starts a thread to read data using read_device
    void stopDataThread();  // Stops the data thread

    std::string getDeviceName() const;

private:
    std::string deviceName;
    std::string devicePath;  // Path to the serial device (e.g., /dev/Onion1)
    std::atomic<bool> keepRunning;  // Atomic flag to control the thread
    std::thread dataThread;  // Thread to handle incoming data

    void readData();  // Thread function to call read_device
};

#endif // ONION_H
