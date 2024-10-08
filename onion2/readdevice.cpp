// read_device.cpp (Example implementation)
#include <iostream>
#include <sqlite3.h>
#include <fcntl.h>    // For opening the port
#include <unistd.h>   // For read/write operations
#include <termios.h>  // For configuring serial port settings
#include <cstring>  // Include for memset


void read_device(const std::string& device_path, sqlite3* db, const std::string& device_name) {
    int fd = open(device_path.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        std::cerr << "Error opening port " << device_path << std::endl;
        return;
    }

    // Configure the serial port settings
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0) {
        std::cerr << "Error getting termios attributes" << std::endl;
        close(fd);
        return;
    }

    cfsetospeed(&tty, B115200);  // Set baud rate
    cfsetispeed(&tty, B115200);
    tty.c_cflag |= (CLOCAL | CREAD);  // Enable receiver, set local mode
    tcsetattr(fd, TCSANOW, &tty);  // Apply the settings

    char buffer[256];
    int n;
    while (true) {
        n = read(fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = 0;
            std::string data(buffer);
            std::cout << "Received data from " << device_name << ": " << data << std::endl;
            // Insert the data into SQLite database or process as needed
        }
    }

    close(fd);
}
