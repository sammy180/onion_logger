#include <iostream>
#include <thread>
#include <vector>
#include <sqlite3.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <chrono>
#include <ctime>
#include <sys/stat.h>
#include "onion.h" // Include our custom header file for function declarations






int main() {
    // Load the headers from a text file
    std::vector<std::string> headers = load_headers("headers.txt");
    if (headers.empty()) {
        std::cerr << "Error: No headers found in the file!" << std::endl;
        return 1;
    }

    sqlite3* db;
    int rc = sqlite3_open("device_data.db", &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    } else {
        std::cout << "Opened database successfully" << std::endl;
    }

    // Create the table with dynamic columns based on headers
    create_table(db, headers);

    std::cout << "Starting the USB data capture program..." << std::endl;

    // Start monitoring devices
    monitor_devices(db, headers);

    sqlite3_close(db);
    std::cout << "Program terminating and database closed." << std::endl;
    return 0;
}



//g++ main.cpp onion.cpp -o logion -pthread -lsqlite3
