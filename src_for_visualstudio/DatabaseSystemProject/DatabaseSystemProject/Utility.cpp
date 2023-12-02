#include <string>
#include <deque>
#include <mutex>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <windows.h>
#define MAX_OPEN_FILE 500

static std::mutex fileMutex;
int fileCount = 0;

std::deque<std::string> getFileLines(const std::wstring& filename) {
    std::ifstream file(filename);
    std::deque<std::string> data;
    //std::string line;

    //while (std::getline(file, line)) {
    //    data.push_back(line);
    //}

    const int MAX_LENGTH = 524288;
    char* line = new char[MAX_LENGTH];

    while (file.getline(line, MAX_LENGTH)) {
        data.push_back(std::string(line));
    }

    return data;
}

std::string wideStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

void printProgress(long counter, long target, bool init) {
    static int prev = -1;

    if (init) {
        prev = -1;
        std::cout << "    ";
    }

    int progress = static_cast<int>(static_cast<double>(counter) / target * 100);

    if (progress != prev) {
        prev = progress;
        std::cout << "\b\b\b\b    \b\b\b\b";
        printf("%3d%%", progress);
    }
}

std::string toBytesFormat(size_t bytes) {
    static const size_t BUF_SIZE = 100;
    static const char* suffix[] = {
        "bytes", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB", "RB", "QB"
    };
    char* buf = (char*) malloc(sizeof(char) * BUF_SIZE);
    double _bytes = static_cast<double>(bytes);
    int i = 0;

    while (_bytes >= 1000) {
        ++i;
        _bytes /= 1000;
    }

    sprintf_s(buf, BUF_SIZE, "%.1f %s", _bytes, suffix[i]);
    return std::string(buf);
}

std::string toTimeFormat(time_t milliseconds) {
    static const size_t BUF_SIZE = 200;

    if (milliseconds < 10000) {
        auto str = std::to_string(milliseconds) + " ms";
        return str.c_str();
    }

    char* buf = (char*) malloc(sizeof(char) * BUF_SIZE);
    int sec = static_cast<int>(milliseconds / 1000);
    int min = sec / 60;
    int hr = min / 60;
    min -= hr * 60;
    sec -= min * 60;

    if (hr > 0) {
        sprintf_s(buf, BUF_SIZE, "%d hr %d min %d sec", hr, min, sec);
    }
    else if (min > 0) {
        sprintf_s(buf, BUF_SIZE, "%d min %d sec", min, sec);
    }
    else {
        sprintf_s(buf, BUF_SIZE, "%d sec", sec);
    }
    
    return std::string(buf);
}

void writeFile(std::ofstream &file, const std::string& filename, std::ios::openmode mode) {
    //fileMutex.lock();
    //int tmpCount = fileCount + 1;
    //fileMutex.unlock();

    //if (tmpCount > MAX_OPEN_FILE) {
        //return false;
    //}

    file.open(filename, mode);
    file.exceptions(std::ios::failbit | std::ifstream::badbit);
}

void closeFile(std::ofstream &file) {
    try {
        file.close();
    }
    catch (...) {
        char message[200];
        strerror_s(message, sizeof(message), errno);
        std::cerr << "Failed to close a file" << ": " << message << std::endl;
        return;
    }

    //fileMutex.lock();
    //--fileCount;
    //fileMutex.unlock();
}
