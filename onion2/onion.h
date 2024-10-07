// onion.h
#ifndef ONION_H
#define ONION_H

#include <string>
#include <iostream>
#include <thread>
#include <atomic>

class Onion {
public:
    Onion(const std::string& deviceName);  // Constructor
    ~Onion();  // Destructor
    
    Onion(const Onion&) = delete;  // Disable copy constructor
    Onion& operator=(const Onion&) = delete;  // Disable copy assignment

    Onion(Onion&& other) noexcept;  // Move constructor
    Onion& operator=(Onion&& other) noexcept;  // Move assignment

    void openPort();  // Simulates opening a port
    void startDataThread();  // Starts a thread to simulate data recording
    void stopDataThread();  // Stops the data thread

    std::string getDeviceName() const;  // Getter for deviceName

private:
    std::string deviceName;
    std::atomic<bool> keepRunning;  // Atomic flag to control the thread
    std::thread dataThread;  // Thread to handle incoming data

    void recordData();  // Function run by the thread to record data
};

#endif // ONION_H
