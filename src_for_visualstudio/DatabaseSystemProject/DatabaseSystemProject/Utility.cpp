#include <string>
#include <vector>
#include <mutex>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <windows.h>

std::vector<std::string> getFileLines(const std::wstring& filename) {
    std::ifstream file(filename);
    std::vector<std::string> data;
    std::string line;

    while (std::getline(file, line)) {
        data.push_back(line);
    }
    file.close();

    return data;
}

// deprecated
std::string wideStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

void printProgress(long target, bool init) {
    static std::mutex m;
    static int counter = 0;
    static int prev = -1;

    m.lock();

    if (init) {
        counter = -1;
        prev = -1;
        std::cout << "    ";
    }

    int progress = static_cast<int>(static_cast<double>(++counter) / target * 100);

    if (progress != prev) {
        prev = progress;
        std::cout << "\b\b\b\b    \b\b\b\b";
        printf("%3d%%", progress);
    }

    m.unlock();
}

std::string toBytesFormat(long bytes) {
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

std::string toTimeFormat(long milliseconds) {
    static const size_t BUF_SIZE = 200;

    if (milliseconds < 10000) {
        auto str = std::to_string(milliseconds) + " ms";
        return str.c_str();
    }

    char* buf = (char*) malloc(sizeof(char) * BUF_SIZE);
    int sec = milliseconds / 1000;
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