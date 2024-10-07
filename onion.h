#ifndef ONION_H
#define ONION_H

#include <vector>
#include <string>
#include <sqlite3.h>
#include <atomic>


// Function declarations

bool user_prompt(const std::string& message);
void monitor_devices(sqlite3* db, const std::vector<std::string>& headers);
std::vector<std::string> split_string(const std::string& str, char delimiter);
void create_table(sqlite3* db, const std::vector<std::string>& headers) ;
std::string sanitize_data(const std::string& data);
//void insert_data(sqlite3* db, const std::string& device_name, const std::vector<std::string>& headers, const std::vector<std::string>& values);
void insert_data(sqlite3* db, const std::string& device_name, const std::vector<std::string>& headers, std::vector<std::string> values);

void read_device(const std::string &device_path, const std::string &device_name,
                 sqlite3* db, const std::vector<std::string>& headers,
                 std::atomic<bool>& keep_running);
std::vector<std::string> load_headers(const std::string& filename);

#endif
