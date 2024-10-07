#include <iostream>
#include <fstream>
#include <vector>
#include <sqlite3.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sstream>
#include <cstring>
#include <sys/stat.h>
#include <algorithm> // For std::remove
#include "onion.h"
#include <sys/inotify.h>
#include <limits.h>
#include <unistd.h>
#include <map>
#include <thread>
#include <atomic>
#include <memory>
#include <atomic>
#include <libudev.h>
#include <set> 
#include "onionsensor.h"

void monitor_devices(sqlite3* db, const std::vector<std::string>& headers) {
    // Initialize udev
    struct udev* udev = udev_new();
    if (!udev) {
        std::cerr << "Can't create udev\n";
        return;
    }

    // Initialize inotify
    int inotifyFd = inotify_init();
    if (inotifyFd == -1) {
        std::cerr << "Failed to initialize inotify: " << strerror(errno) << std::endl;
        udev_unref(udev);
        return;
    }

    // Watch for changes in the /dev directory
    int wd = inotify_add_watch(inotifyFd, "/dev", IN_CREATE | IN_DELETE);
    if (wd == -1) {
        std::cerr << "Failed to add inotify watch: " << strerror(errno) << std::endl;
        close(inotifyFd);
        udev_unref(udev);
        return;
    }

    // Map to hold the OnionSensor instances
    std::map<std::string, std::unique_ptr<OnionSensor>> sensors;

    // Set of device nodes to monitor
    std::set<std::string> device_nodes = {
        "/dev/Onion1",
        "/dev/Onion2",
        "/dev/Onion3",
        "/dev/Onion4"
    };

    // **Initial Scan to Detect Connected Devices**
    struct udev_enumerate* enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "tty");
    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry* dev_list_entry;

    udev_list_entry_foreach(dev_list_entry, devices) {
        const char* path = udev_list_entry_get_name(dev_list_entry);
        struct udev_device* dev = udev_device_new_from_syspath(udev, path);

        const char* devnode = udev_device_get_devnode(dev);
        if (devnode && device_nodes.count(devnode)) {
            std::string device_name = devnode;
            std::cout << "[INFO] Device connected at startup: " << device_name << std::endl;

            // Use user prompt or zenity as before
            bool user_response = user_prompt("Device " + device_name + " is connected. Include in data recording?");
            if (user_response) {
                // Create a new OnionSensor instance
                sensors[device_name] = std::unique_ptr<OnionSensor>(new OnionSensor(devnode, device_name, db, headers));
                std::cout << "[INFO] Started sensor for " << device_name << std::endl;
            } else {
                std::cout << "[INFO] Not including " << device_name << " in data recording." << std::endl;
            }
        }

        udev_device_unref(dev);
    }

    udev_enumerate_unref(enumerate);

    // Begin monitoring for device events
    char buf[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
    ssize_t numRead;

    while (true) {
        numRead = read(inotifyFd, buf, sizeof(buf));
        if (numRead <= 0) {
            std::cerr << "Error reading inotify events: " << strerror(errno) << std::endl;
            break;
        }

        for (char* p = buf; p < buf + numRead; ) {
            struct inotify_event* event = (struct inotify_event*) p;
            std::string device_name = event->name;

            // Debugging output...

            if (event->mask & IN_CREATE) {
                // Check for devices in device_nodes
                if (device_name.find("ttyUSB") == 0 || device_name.find("Onion") == 0) {
                    usleep(500000); // Wait for symlink creation

                    for (const auto& onion_device : device_nodes) {
                        struct stat sb;
                        if (stat(onion_device.c_str(), &sb) == 0 && S_ISCHR(sb.st_mode)) {
                            std::string device_short_name = onion_device.substr(5); // Extract OnionX

                            // If sensor not already created
                            if (sensors.count(device_short_name) == 0) {
                                std::cout << "[INFO] Device plugged in: " << onion_device << std::endl;

                                // Prompt user
                                bool user_response = user_prompt("Device " + device_short_name + " plugged in. Include in data recording?");
                                if (user_response) {
                                    // Create a new OnionSensor instance
                                    sensors[device_short_name] = std::unique_ptr<OnionSensor>(new OnionSensor(onion_device, device_short_name, db, headers));
                                    std::cout << "[INFO] Started sensor for " << device_short_name << std::endl;
                                } else {
                                    std::cout << "[INFO] Not including " << device_short_name << " in data recording." << std::endl;
                                }
                            }
                        }
                    }
                }
            } else if (event->mask & IN_DELETE) {
                // Handle device removal
                std::string device_path = "/dev/" + device_name;
                if (device_nodes.count(device_path)) {
                    std::string device_short_name = device_name;
                    std::cout << "[INFO] Device unplugged: " << device_path << std::endl;

                    // Destroy the OnionSensor instance
                    if (sensors.count(device_short_name)) {
                        std::cout << "[INFO] Stopping sensor for " << device_short_name << std::endl;
                        sensors.erase(device_short_name);
                        std::cout << "[INFO] Sensor for " << device_short_name << " stopped." << std::endl;
                    }
                }
            }

            p += sizeof(struct inotify_event) + event->len;
        }
    }

    // Cleanup
    inotify_rm_watch(inotifyFd, wd);
    close(inotifyFd);
    udev_unref(udev);

    // Sensors will be automatically destroyed when the map goes out of scope
}




bool user_prompt(const std::string& message) {
    std::string response;
    while (true) {
        std::cout << message << " [y/n]: ";
        std::getline(std::cin, response);
        if (response == "y" || response == "Y") {
            return true;
        } else if (response == "n" || response == "N") {
            return false;
        } else {
            std::cout << "Please enter 'y' for yes or 'n' for no." << std::endl;
        }
    }
}






// Function to sanitize input by removing newline and carriage return characters
std::string sanitize_data(const std::string& data) {
    std::string sanitized = data;
    // Remove \n (newline) and \r (carriage return) characters
    sanitized.erase(std::remove(sanitized.begin(), sanitized.end(), '\n'), sanitized.end());
    sanitized.erase(std::remove(sanitized.begin(), sanitized.end(), '\r'), sanitized.end());
    return sanitized;
}



// Function to split the semicolon-separated string
std::vector<std::string> split_string(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

// Function to create a table with dynamic columns based on headers
void create_table(sqlite3* db, const std::vector<std::string>& headers) {
    std::string sql = "CREATE TABLE IF NOT EXISTS device_data ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "device_name TEXT NOT NULL, "
                      "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, ";

    // Add columns based on the headers
    for (size_t i = 0; i < headers.size(); ++i) {
        sql += headers[i] + " TEXT";
        if (i < headers.size() - 1) {
            sql += ", ";
        }
    }

    sql += ");";

    char* errMsg = 0;
    int rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error while creating table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    } else {
        std::cout << "Table created successfully or already exists." << std::endl;
    }
}


// Function to insert labeled data into the SQLite database
// Function to insert labeled data into the SQLite database
void insert_data(sqlite3* db, const std::string& device_name, const std::vector<std::string>& headers, std::vector<std::string> values) {
    // If there are more headers than values, pad the remaining headers with NULL
    while (values.size() < headers.size()) {
        values.push_back("NULL");
    }

    std::string sql = "INSERT INTO device_data (device_name, timestamp, ";
    
    // Add headers as column names
    for (size_t i = 0; i < headers.size(); ++i) {
        sql += headers[i];
        if (i < headers.size() - 1) {
            sql += ", ";
        }
    }
    
    sql += ") VALUES ('" + device_name + "', datetime('now'), ";

    // Add values for each column
    for (size_t i = 0; i < values.size(); ++i) {
        if (values[i] == "NULL") {
            sql += "NULL";
        } else {
            sql += "'" + values[i] + "'";
        }
        if (i < values.size() - 1) {
            sql += ", ";
        }
    }

    sql += ");";

    char* errMsg = 0;
    int rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error while inserting data: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    } else {
        std::cout << "Inserted data for device: " << device_name << std::endl;
    }
}


// Function to read data from a device and insert labeled data into the SQLite database

void read_device(const std::string &device_path, const std::string &device_name,
                 sqlite3* db, const std::vector<std::string>& headers,
                 std::atomic<bool>& keep_running) {
    std::cout << "Starting thread for device: " << device_path << std::endl;
    std::cout.flush();

    // Open the serial port
    int fd = open(device_path.c_str(), O_RDWR | O_NOCTTY);
    if (fd == -1) {
        std::cerr << "Failed to open " << device_path << ": " << strerror(errno) << std::endl;
        return;
    } else {
        std::cout << "Serial port " << device_path << " opened successfully." << std::endl;
        std::cout.flush();
    }

    // Set serial port settings
    struct termios options;
    tcgetattr(fd, &options);
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

    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        std::cerr << "Failed to set serial port attributes on " << device_path << ": " << strerror(errno) << std::endl;
        close(fd);
        return;
    }

    char buffer[256];
    ssize_t n;
    std::string incomplete_data;  // Buffer for storing incomplete data

    while (keep_running) {
        // Read data from the serial port
        n = read(fd, buffer, sizeof(buffer) - 1);
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
            std::cout << "Read from " << device_path << ": " << data << std::endl;
            std::cout.flush();  // Ensure the output is flushed immediately

            // Split the data string by semicolon
            std::vector<std::string> values = split_string(data, ';');

            // If there are fewer tokens than headers, buffer the data
            if (values.size() < 30) {
                std::cerr << "Data incomplete: less tokens than headers, buffering incomplete data." << std::endl;
                std::cerr.flush();
                incomplete_data = data;  // Store incomplete data in the buffer
                continue;
            }

            // Insert the labeled data into the SQLite database
            insert_data(db, device_name, headers, values);
            std::cout << "Inserted data for device: " << device_name << std::endl;
            std::cout.flush();
        } else if (n == 0) {
            // No data received
            usleep(100000); // Sleep for 100 milliseconds to prevent tight looping
        } else {
            std::cerr << "Error reading from " << device_path << ": " << strerror(errno) << std::endl;
            std::cerr.flush();
            break;
        }
    }

    close(fd);
    std::cout << "Thread for device " << device_path << " terminating." << std::endl;
    std::cout.flush();
}

// Function to load headers from a text file
std::vector<std::string> load_headers(const std::string& filename) {
    std::ifstream file(filename);
    std::vector<std::string> headers;
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Error: Could not open the file " << filename << std::endl;
        return headers;
    }

    while (std::getline(file, line)) {
        if (!line.empty()) {
            headers.push_back(line);
        }
    }

    file.close();
    return headers;
}
