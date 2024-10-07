#ifndef ONION_SENSOR_H
#define ONION_SENSOR_H

#include <string>
#include <thread>
#include <atomic>
#include <sqlite3.h>
#include <vector>

class OnionSensor {
public:
    OnionSensor(const std::string& device_path, const std::string& device_name,
                sqlite3* db, const std::vector<std::string>& headers);
    ~OnionSensor();

    void start();
    void stop();

private:
    void read_loop();

    std::string device_path_;
    std::string device_name_;
    sqlite3* db_;
    std::vector<std::string> headers_;

    int fd_;  // File descriptor for the serial port
    std::thread read_thread_;
    std::atomic<bool> keep_running_;
};

#endif // ONION_SENSOR_H
