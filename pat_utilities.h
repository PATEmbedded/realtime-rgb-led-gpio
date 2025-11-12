#ifndef pat_jsonFile_H
#define pat_jsonFile_H

#include <iostream>
#include <fstream>
#include <ctime>
#include <fcntl.h>
#include <sys/file.h>
#include <nlohmann/json.hpp>
#include <csignal>  
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <unistd.h>   // for getpid(), kill()
#include <signal.h>   // for SIGTERM
#include <cstdio>     // for popen(), pclose()
#include <cstdlib>    // for exit()
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <filesystem>

#include <fstream>

#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <filesystem>

#define _path_cpp "rgb_config_cpp.json"
#define _path_nodejs "rgb_config_nodejs.json"
#define _driver_name "pat_rgb_driver"

#pragma once
#define ENABLE_LOGS 0  // Set 1 to enable logs, 0 to disable

#if ENABLE_LOGS
  #define LOG_PRINTF(...) printf(__VA_ARGS__)
  #define LOG_COUT std::cout
  #define LOG_CERR std::cerr
  #define LOG_PERROR(msg) perror(msg)
#else
  #define LOG_PRINTF(...) ((void)0)
  #define LOG_COUT if (false) std::cout
  #define LOG_CERR if (false) std::cerr
  #define LOG_PERROR(msg) ((void)0)
#endif



#define DL {printf("%d\n", __LINE__); delay(100);}
#define msleep(xxx)  std::this_thread::sleep_for(std::chrono::milliseconds(xxx));


using Json = nlohmann::json;
using namespace std;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Json readJson(const std::string& filename) {
    Json j;
    const int maxRetries = 10;
    const int retryDelayMs = 200;  // 400 ms delay between retries

    for (int attempt = 0; attempt < maxRetries; ++attempt) {
        int fd = open(filename.c_str(), O_RDONLY);
        if (fd == -1) {
            LOG_CERR << "Attempt " << attempt + 1 << ": Failed to open " << filename << std::endl;
            usleep(retryDelayMs * 1000);
            continue;
        }

        if (flock(fd, LOCK_SH) != 0) {
            LOG_CERR << "Attempt " << attempt + 1 << ": Failed to acquire shared lock on " << filename << std::endl;
            close(fd);
            usleep(retryDelayMs * 1000);
            continue;
        }

        std::ifstream file(filename);
        if (!file.is_open()) {
            LOG_CERR << "Attempt " << attempt + 1 << ": Failed to open stream for " << filename << std::endl;
            flock(fd, LOCK_UN);
            close(fd);
            usleep(retryDelayMs * 1000);
            continue;
        }

        try {
            file >> j;
            flock(fd, LOCK_UN);
            close(fd);
            return j;
        } catch (const std::exception& e) {
            LOG_CERR << "Attempt " << attempt + 1 << ": JSON parse error: " << e.what() << std::endl;
            file.close();
            flock(fd, LOCK_UN);
            close(fd);
            usleep(retryDelayMs * 1000);
        }
    }

    LOG_CERR << "❌ Failed to read and parse " << filename << " after " << maxRetries << " attempts." << std::endl;
    return {};  // Return empty JSON if all attempts fail
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <string>
#include <optional>
struct RequestData {
    std::string command = "idle";
    int red =0;
    int green=0;
    int blue=0;
    string updated;


    std::string filePath;
    std::filesystem::file_time_type lastWriteTime;
    std::mutex mtx;
    //----------------------------------------------------
    RequestData(const std::string& path) : filePath(path) {
           lastWriteTime = std::filesystem::file_time_type::min();
            command = "idle";
            red =0;
            green=0;
            blue=0;
            updated="2000-01-01T100:00:00";
    }
    //----------------------------------------------------
    void updateFromJson(const Json& json) {
    
        auto it = json.find("command");
        if (it != json.end() && it->is_string())
            command = it->get<std::string>();
        else command = "idle";

        it = json.find("red");
        if (it != json.end() && !it->is_null()){
            red = it->get<int>();
            if(red<0)red=0;
            if(red>255)red=255;
        }
        else{
            red = 0;
        }

        it = json.find("green");
        if (it != json.end() && !it->is_null()){
            green = it->get<int>();
            if(green<0)green=0;
            if(green>255)green=255;
        }
        else{
            green = 0;
        }

        it = json.find("blue");
        if (it != json.end() && !it->is_null()){
            blue = it->get<int>();
            if(blue<0)blue=0;
            if(blue>255)blue=255;
        }
        else{
            blue = 0;
        }

        it = json.find("updated");
        if (it != json.end() && it->is_string())
            updated = it->get<std::string>();
        else updated="2000-01-01T100:00:00";
    }
//----------------------------------------------------
    bool changed() {
        std::lock_guard<std::mutex> lock(mtx);
        if (!std::filesystem::exists(filePath)) return false;

        auto currentWriteTime = std::filesystem::last_write_time(filePath);
        if (currentWriteTime == lastWriteTime) return false;

        lastWriteTime = currentWriteTime;

        this->updateFromJson(readJson(filePath));

        return true; 
    }
//----------------------------------------------------
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool killDuplicateProcesses(const std::string& fileName) {
    pid_t currentPid = getpid();
    std::string command = "ps -eo pid,cmd | grep -v grep | grep " + fileName;

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return false;

    char buffer[512];
    std::vector<pid_t> pids;

    while (fgets(buffer, sizeof(buffer), pipe)) {
        std::istringstream iss(buffer);
        pid_t pid = 0;
        std::string cmd;
        if (!(iss >> pid)) continue;
        std::getline(iss, cmd);
        if (pid != currentPid) {
            pids.push_back(pid);
        }
    }
    pclose(pipe);

    for (pid_t pid : pids) {
        if (kill(pid, SIGTERM) == -1) {
            LOG_PERROR(("Failed to kill pid " + std::to_string(pid)).c_str());
        }
    }

    return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
string getTimestamp() {
    time_t now = time(0);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", localtime(&now));
    return string(buf);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
time_t parseTimestamp(const string& timeStr) {
    std::tm tm = {};
    std::istringstream ss(timeStr);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (ss.fail()) {
        LOG_CERR << "Failed to parse timestamp: " << timeStr << std::endl;
        return 0;
    }
    return std::mktime(&tm);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void deepMerge(Json& target, const Json& source, size_t maxArraySize = 10) {
    for (auto it = source.begin(); it != source.end(); ++it) {
        const std::string& key = it.key();

        if (target.find(key) != target.end()) {
            if (target[key].is_object() && it.value().is_object()) {
                // Recursive merge for objects
                deepMerge(target[key], it.value(), maxArraySize);
            }
            else if (target[key].is_array() && it.value().is_array()) {
                // Prepend new array elements from source to target
                for (auto rit = it.value().rbegin(); rit != it.value().rend(); ++rit) {
                    target[key].insert(target[key].begin(), *rit);
                }

                // Limit size of array after merge
                if (target[key].size() > maxArraySize) {
                    target[key].erase(target[key].begin() + maxArraySize, target[key].end());
                }
            }
            else {
                // Replace value for primitives or mismatched types
                target[key] = it.value();
            }
        } else {
            // Key does not exist: just insert
            target[key] = it.value();
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



bool replaceJsonFile(const string &path, Json json) {
    int fd = open(path.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        LOG_CERR << "Cannot open " << path << " for writing." << endl;
        return false;
    }

    if (flock(fd, LOCK_EX) != 0) {
        LOG_CERR << "Cannot acquire lock on " << path << endl;
        close(fd);
        return false;
    }

    // فقط json جدید رو بنویس، قبلی‌ها رو حذف کن
    ofstream outFile(path, ios::trunc); // فایل رو پاک می‌کنه
    if (!outFile.is_open()) {
        LOG_CERR << "Cannot open " << path << " for writing." << endl;
        flock(fd, LOCK_UN);
        close(fd);
        return false;
    }

    outFile << json.dump(4); // pretty print با ۴ فاصله
    outFile.close();

    flock(fd, LOCK_UN);
    close(fd);
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::time_t getBootTime() {
    std::ifstream uptimeFile("/proc/uptime");
    double uptimeSeconds = 0;
    if (uptimeFile >> uptimeSeconds) {
        return time(0) - (std::time_t)uptimeSeconds;
    }
    return 0;
}

bool isRecentlyBooted() {
    Json doc = readJson(_path_nodejs);
    std::string str = doc["updated"].get<std::string>();
    time_t updatedRequest = parseTimestamp(str);
    time_t bootTime = getBootTime();
    time_t now = std::time(nullptr);
    LOG_COUT << "Updated Request Time: " << std::asctime(std::localtime(&updatedRequest));
    LOG_COUT << "Boot Time           : " << std::asctime(std::localtime(&bootTime));
    LOG_COUT << "Current Time        : " << std::asctime(std::localtime(&now));
    if (updatedRequest == 0) {
        LOG_CERR << "Error: Timestamp parse failed.\n";
        return false;
    }
    return updatedRequest < bootTime;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool updateJsonFile(const string &path ,Json json) {
    Json j;
    int fd = open(path.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        LOG_CERR << "Cannot open " << path << " for writing." << endl;
        return false;
    }

    if (flock(fd, LOCK_EX) != 0) {
        LOG_CERR << "Cannot acquire lock on " << path << endl;
        close(fd);
        return false;
    }

    // Read existing content if any
    ifstream inFile(path);
    if (inFile.is_open()) {
        try {
            inFile >> j;
        } catch (...) {
            j = Json::object();
        }
        inFile.close();
    } else {
        j = Json::object();
    }

    // Deep merge input json into j
    deepMerge(j, json);

    // Write updated content back to the file
    ofstream outFile(path, ios::trunc); // truncate file first
    if (!outFile.is_open()) {
        LOG_CERR << "Cannot reopen " << path << " for writing." << endl;
        flock(fd, LOCK_UN);
        close(fd);
        return false;
    }

    outFile << j.dump(4); // pretty print with 4 spaces
    outFile.close();

    flock(fd, LOCK_UN);
    close(fd);
    return true;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int findFreeID(const Json& middleware) {
    for (int i = 1; i < 256; ++i) {
        if (middleware.find(to_string(i)) == middleware.end()) {
            return i;
        }
    }
    return -1;  // no free ID
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <optional>
#include <tuple>
#include <set>      
#include <string>   

// std::optional<std::pair<int, int>> getUserOrderFromId (const Json& j,int fingerprint_id) {
//     std::string idStr = std::to_string(fingerprint_id);

//     if (j.find(idStr) != j.end()) {
//         int userId = j[idStr]["userId"];
//         int orderId = j[idStr]["orderId"];
//         return std::make_pair(userId, orderId);
//     }
//     return std::nullopt;
// }

int getUserIdFromId (const Json& j,int fingerprint_id) {
    std::string idStr = std::to_string(fingerprint_id);
    int userId=-1;
    if (j.find(idStr) != j.end()) {
        userId = j[idStr]["userId"].get<int>();
        return userId;
    }
    return -1;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int getIdFromUserOrder(const Json& j, int userId, int orderId) {
    LOG_PRINTF("[getIdFromUserOrder] Searching for userId=%d and orderId=%d\n", userId, orderId);
    for (auto it = j.begin(); it != j.end(); ++it) {
        const auto& val = it.value();
        LOG_PRINTF("[getIdFromUserOrder] Checking key: %s\n", it.key().c_str());
        if (val.find("userId") != val.end() && val.find("orderId") != val.end()) {
            LOG_PRINTF("[getIdFromUserOrder] Found userId=%d, orderId=%d in JSON\n", val["userId"].get<int>(), val["orderId"].get<int>());
            if (val["userId"] == userId && val["orderId"] == orderId) {
                LOG_PRINTF("[getIdFromUserOrder] Match found! Returning ID = %s\n", it.key().c_str());
                return std::stoi(it.key());
            }
        }
    }
    LOG_PRINTF("[getIdFromUserOrder] No match found.\n");
    return -1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int getSmallestMissingId(const Json& j) {
    const int MAX_ID = 512;
    std::vector<bool> exists(MAX_ID, false);
    LOG_PRINTF("[getSmallestMissingId] Scanning keys...\n");

    for (auto it = j.begin(); it != j.end(); ++it) {
        int id = std::stoi(it.key());
        if (id >= 0 && id < MAX_ID) {
            exists[id] = true;
            LOG_PRINTF("[getSmallestMissingId] Key exists: %d\n", id);
        }
    }

    for (int i = 0; i < MAX_ID; ++i) {
        if (!exists[i]) {
            LOG_PRINTF("[getSmallestMissingId] Smallest missing ID found: %d\n", i);
            return i;
        }
    }

    LOG_PRINTF("[getSmallestMissingId] All IDs 0-%d are used. Returning -1\n", MAX_ID - 1);
    return -1; // All IDs are used
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int addUserOrderToId(Json& j, int userId, int orderId) {
    LOG_PRINTF("[addUserOrderToId] Adding userId=%d, orderId=%d\n", userId, orderId);
    int newId = getSmallestMissingId(j);
    if (newId == -1) {
        LOG_PRINTF("[addUserOrderToId] Error: No available ID slots!\n");
        return newId;
    }
    j[std::to_string(newId)] = {
        { "userId", userId },
        { "orderId", orderId }
    };
    LOG_PRINTF("[addUserOrderToId] Added at ID=%d\n", newId);
    return newId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool removeUserOrderFromId(Json& j, int id) {
    std::string key = std::to_string(id);
    LOG_PRINTF("[removeUserOrderFromId] Attempting to remove ID=%d\n", id);
    if (j.find(key) != j.end()) {
        j.erase(key);
        LOG_PRINTF("[removeUserOrderFromId] Successfully removed ID=%d\n", id);
        return true;
    }
    LOG_PRINTF("[removeUserOrderFromId] ID=%d not found\n", id);
    return false;
}


#endif  //pat_jsonFile_H
