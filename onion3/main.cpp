#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <string>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <sqlite3.h>
#include <sstream>
#include <algorithm>
#include <dirent.h>
#include <fstream>

using namespace std;

class SerialMonitor {
public:
    SerialMonitor(const string& portName, sqlite3* db, const vector<string>& headers) 
        : port(portName), db(db), headers(headers), running(false) {}

    // Start monitoring in a separate thread
    void start() {
        running = true;
        monitorThread = thread(&SerialMonitor::monitorSerial, this);
    }

    // Stop monitoring
    void stop() {
        running = false;
        if (monitorThread.joinable()) {
            monitorThread.join();  // Wait for the thread to finish
        }
    }

    ~SerialMonitor() {
        stop();  // Ensure thread is stopped before destruction
    }

private:
    void monitorSerial() {
        // Adding initial delay to allow the device to be ready
        cout << "Initial delay before starting to read from device." << endl;
        this_thread::sleep_for(chrono::seconds(1));

        while (running) {
            lock_guard<mutex> lock(dataMutex);
            // Call the method to read data from the serial device
            readDevice();

            this_thread::sleep_for(chrono::milliseconds(100)); // Simulate reading delay
        }
    }

    void readDevice() {
        cout << "Starting thread for device: " << port << endl;
        cout.flush();

        // Open the serial port
        int fd = open(port.c_str(), O_RDWR | O_NOCTTY);
        if (fd == -1) {
            cerr << "Failed to open " << port << ": " << strerror(errno) << endl;
            return;
        } else {
            cout << "Serial port " << port << " opened successfully." << endl;
            cout.flush();
        }

        // Set serial port settings
        struct termios options;
        if (tcgetattr(fd, &options) != 0) {
            cerr << "Failed to get serial port attributes on " << port << ": " << strerror(errno) << endl;
            close(fd);
            return;
        }
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
        options.c_cc[VMIN] = 10;  // Wait until at least 10 bytes are received
        options.c_cc[VTIME] = 1;  // Set the timeout to 1 decisecond (100 ms)

        if (tcsetattr(fd, TCSANOW, &options) != 0) {
            cerr << "Failed to set serial port attributes on " << port << ": " << strerror(errno) << endl;
            close(fd);
            return;
        }

        char buffer[1024];  // Increased buffer size to handle larger messages
        ssize_t n;
        string accumulated_data;

        while (running) {
            cout << "Attempting to read from device: " << port << endl;
            cout.flush();

            // Read data from the serial port
            n = read(fd, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                buffer[n] = '\0';
                accumulated_data += buffer;  // Accumulate the incoming data

                // Sanitize accumulated data by removing newlines and carriage returns
                accumulated_data.erase(remove(accumulated_data.begin(), accumulated_data.end(), '\n'), accumulated_data.end());
                accumulated_data.erase(remove(accumulated_data.begin(), accumulated_data.end(), '\r'), accumulated_data.end());
                accumulated_data.erase(remove(accumulated_data.begin(), accumulated_data.end(), ' '), accumulated_data.end());

                // Remove the last semicolon if it exists
                if (!accumulated_data.empty() && accumulated_data.back() == ';') {
                    accumulated_data.pop_back();
                }

                // Display accumulated data for debugging
                cout << "Accumulated data: " << accumulated_data << endl;

                // Check if the accumulated data contains a complete message
                if (accumulated_data.find("BOX") == 0 && accumulated_data.rfind("X") == accumulated_data.size() - 1) {
                    // We have a complete message
                    cout << "Complete message received: " << accumulated_data << endl;

                    // Split the data by ';' and insert it into the database
                    vector<string> values = splitData(accumulated_data);
                    if (values.size() == headers.size()) {
                        insertData(db, port, headers, values);
                    } else {
                        cerr << "Data incomplete or does not match header count." << endl;
                        cerr.flush();
                    }

                    // Clear the accumulated data after processing
                    accumulated_data.clear();
                }
            } else if (n == 0) {
                cout << "No data received from " << port << ", sleeping..." << endl;
                cout.flush();
                // No data received
                usleep(100000); // Sleep for 100 milliseconds to prevent tight looping
            } else {
                cerr << "Error reading from " << port << ": " << strerror(errno) << endl;
                cerr.flush();
                break;
            }
        }

        close(fd);
        cout << "Thread for device " << port << " terminating." << endl;
        cout.flush();
    }

    vector<string> splitData(const string& data) {
        vector<string> values;
        string sanitizedData = data;
        if (!sanitizedData.empty() && sanitizedData.back() == ';') {
            sanitizedData.pop_back(); // Remove the last semicolon
        }
        stringstream ss(sanitizedData);
        string item;
        while (getline(ss, item, ';')) {
            values.push_back(item);
        }
        return values;
    }

    void insertData(sqlite3* db, const string& deviceName, const vector<string>& headers, const vector<string>& values) {
        // Create table if it does not exist
        string createTableQuery = "CREATE TABLE IF NOT EXISTS SensorData (";
        for (size_t i = 0; i < headers.size(); ++i) {
            createTableQuery += headers[i] + " TEXT";
            if (i < headers.size() - 1) {
                createTableQuery += ", ";
            }
        }
        createTableQuery += ");";
        char* errMsg = nullptr;
        if (sqlite3_exec(db, createTableQuery.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
            cerr << "Failed to create table: " << errMsg << endl;
            sqlite3_free(errMsg);
            return;
        }

        // Insert data into the table
        string insertQuery = "INSERT INTO SensorData (";
        for (size_t i = 0; i < headers.size(); ++i) {
            insertQuery += headers[i];
            if (i < headers.size() - 1) {
                insertQuery += ", ";
            }
        }
        insertQuery += ") VALUES (";
        for (size_t i = 0; i < values.size(); ++i) {
            insertQuery += "'" + values[i] + "'";
            if (i < values.size() - 1) {
                insertQuery += ", ";
            }
        }
        insertQuery += ");";

        if (sqlite3_exec(db, insertQuery.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
            cerr << "Failed to insert data: " << errMsg << endl;
            sqlite3_free(errMsg);
        } else {
            cout << "Data inserted successfully for device: " << deviceName << endl;
            cout.flush();
        }
    }

    string port;
    thread monitorThread;
    atomic<bool> running;
    mutex dataMutex;
    sqlite3* db;
    vector<string> headers;
};

int main() {
    sqlite3* db = nullptr;
    if (sqlite3_open("sensor_data.db", &db) != SQLITE_OK) {
        cerr << "Failed to open database: " << sqlite3_errmsg(db) << endl;
        return 1;
    }

    // Read headers from file
    vector<string> headers;
    ifstream headersFile("headers.txt");
    if (!headersFile) {
        cerr << "Failed to open headers.txt" << endl;
        return 1;
    }
    string header;
    while (getline(headersFile, header)) {
        headers.push_back(header);
    }

    vector<unique_ptr<SerialMonitor>> monitors;
    DIR* dir = opendir("/dev");
    if (dir == nullptr) {
        cerr << "Failed to open /dev directory." << endl;
        return 1;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        string deviceName = entry->d_name;
        if (deviceName.find("Onion") != string::npos && (deviceName == "Onion1" || deviceName == "Onion2" || deviceName == "Onion3" || deviceName == "Onion4")) {
            string fullPath = "/dev/" + deviceName;
            cout << "Found device: " << fullPath << endl;
            monitors.emplace_back(make_unique<SerialMonitor>(fullPath, db, headers));
            monitors.back()->start();
        }
    }
    closedir(dir);

    if (monitors.empty()) {
        cout << "No devices found." << endl;
    }

    // Keep the program running indefinitely until terminated
    cout << "Press Enter to stop monitoring..." << endl;
    cin.get();  // Wait for user input to stop

    // Stop all monitoring threads
    for (auto& monitor : monitors) {
        monitor->stop();
    }

    sqlite3_close(db);
    return 0;
}