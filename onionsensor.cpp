#include "onionsensor.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <sstream>
#include <algorithm>

extern void insert_data(sqlite3* db, const std::string& device_name, const std::vector<std::string>& headers, std::vector<std::string> values);
extern std::string sanitize_data(const std::string& data);
extern std::vector<std::string> split_string(const std::string& str, char delimiter);

OnionSensor::OnionSensor(const std::string& device_path, const std::string& device_name,
                         sqlite3* db, const std::vector<std::string>& headers)
    : device_path_(device_path), device_name_(device_name), db_(db), headers_(headers), fd_(-1), keep_running_(false) {
    // Open the serial port
    fd_ = open(device_path_.c_str(), O_RDWR | O_NOCTTY);
    if (fd_ == -1) {
        std::cerr << "Failed to open " << device_path_ << ": " << strerror(errno) << std::endl;
        return;
    } else {
        std::cout << "Serial port " << device_path_ << " opened successfully." << std::endl;
    }

    // Set serial port settings
    struct termios options;
    tcgetattr(fd_, &options);
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_cflag &= ~CRTSCTS;
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag &= ~OPOST;
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 10;

    if (tcsetattr(fd_, TCSANOW, &options) != 0) {
        std::cerr << "Failed to set serial port attributes on " << device_path_ << ": " << strerror(errno) << std::endl;
        close(fd_);
        fd_ = -1;
        return;
    }

    // Start the reading thread
    keep_running_ = true;
    read_thread_ = std::thread(&OnionSensor::read_loop, this);
}

OnionSensor::~OnionSensor() {
    stop();
    if (read_thread_.joinable()) {
        read_thread_.join();
    }
    if (fd_ != -1) {
        close(fd_);
    }
    std::cout << "OnionSensor for " << device_name_ << " destroyed." << std::endl;
}

void OnionSensor::start() {
    if (!keep_running_ && fd_ != -1) {
        keep_running_ = true;
        read_thread_ = std::thread(&OnionSensor::read_loop, this);
    }
}

void OnionSensor::stop() {
    keep_running_ = false;
}

void OnionSensor::read_loop() {
    std::cout << "Reading thread started for device: " << device_name_ << std::endl;
    char buffer[256];
    ssize_t n;
    std::string incomplete_data;

    while (keep_running_) {
        // Read data from the serial port
        n = read(fd_, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            std::string data(buffer);

            // Concatenate any incomplete data from previous reads
            if (!incomplete_data.empty()) {
                data = incomplete_data + data;
                incomplete_data.clear();
            }

            // Sanitize the data (remove newline and carriage return characters)
            data = sanitize_data(data);

            // Output the data being read
            std::cout << "Read from " << device_path_ << ": " << data << std::endl;

            // Split the data string by semicolon
            std::vector<std::string> values = split_string(data, ';');

            // If there are fewer tokens than headers, buffer the data
            if (values.size() < headers_.size()) {
                std::cerr << "Data incomplete: less tokens than headers, buffering incomplete data." << std::endl;
                incomplete_data = data;
                continue;
            }

            // Insert the labeled data into the SQLite database
            insert_data(db_, device_name_, headers_, values);
            std::cout << "Inserted data for device: " << device_name_ << std::endl;
        } else if (n == 0) {
            // No data received
            usleep(100000); // Sleep for 100 milliseconds to prevent tight looping
        } else {
            std::cerr << "Error reading from " << device_path_ << ": " << strerror(errno) << std::endl;
            break;
        }
    }

    std::cout << "Reading thread terminating for device: " << device_name_ << std::endl;
}
