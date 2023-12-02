#include <filesystem>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <windows.h>
#include "MapReduce.hpp"
#include "WordCount.hpp"
#include "Profiler.hpp"
#include "Utility.hpp"

namespace fs = std::filesystem;

bool getFileNameFromDialog(std::wstring& outFilename) {
    wchar_t filename[MAX_PATH] = { 0 };
    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;  // If you have a window handle, put it here
    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = L"txt";

    if (GetOpenFileName(&ofn)) {
        outFilename = filename;
        return true;
    }
    return false;
}

int run() {
    // Change system locale
    auto locale = std::locale(".utf-8");
    std::locale::global(locale);    // Set global locale as UTF-8
    std::wcout.imbue(locale);       // Apply locale to wcout
    std::wcin.imbue(locale);        // Apply locale to wcin
    SetConsoleOutputCP(CP_UTF8);    // Apply locale to console

    // Prompt to select a file
    std::wstring inputFilename;
    std::cout << "Please select a file..." << std::endl;

    if (!getFileNameFromDialog(inputFilename)) {
        std::cout << "No file was selected." << std::endl;
        return 1;
    }

    auto lines = getFileLines(inputFilename);
    auto filename = fs::path(inputFilename).filename().string();
    auto space = filename.length();
    size_t size = fs::file_size(inputFilename);
    
    // Print file info
    std::cout << std::endl;
    std::cout << std::setw(space) << "File name" << std::setw(16) << "File size" << std::setw(12) << "Lines" << std::endl;
    std::cout << std::setw(space) << filename << std::setw(16) << toBytesFormat(size) << std::setw(12) << lines.size() << std::endl;

    // Prompt to select batch size
    int batch = 0;

    while (batch < 1) {
        std::cout << std::endl;
        std::cout << "Batch size: ";
        std::cin >> batch;
    }

    // Prompt to select sorting algorithm
    int mode = 0;

    while (mode < 1 || mode > 2) {
        std::cout << std::endl;
        std::cout << "[1] In-place sorting (Fast, high RAM usage)" << std::endl;
        std::cout << "[2] External sorting (Slow, high I/O usage)" << std::endl;
        std::cout << "Select sorting algorithm: ";
        std::cin >> mode;
    }

    bool inplace = (mode == 1);
    int m = 2;

    if (!inplace) {
        std::string str;
        std::cout << std::endl;
        std::cout << "Select m-way merge (default=2): ";
        std::cin.ignore(INT_MAX, '\n');
        std::getline(std::cin, str);
        std::cout << std::endl;

        try {
            m = std::stoi(str);
        }
        catch (...) {
            m = 2;
        }
    }

    // Start benchmarking
    auto& profiler = Profiler::start();

    // Run map-reduce application
    countWords(lines, inplace, m, batch);

    // Aggregate benchmarking data
    auto benchmark = profiler.finish();

    std::cout << std::endl;
    std::cout << "Elapsed time:\t" << toTimeFormat(benchmark.elapsedTime) << std::endl;
    std::cout << "Maximum memory:\t" << toBytesFormat(benchmark.maxMemory) << std::endl;
    std::cout << "Average memory:\t" << toBytesFormat(benchmark.avgMemory) << std::endl;
}

int main() {
    try {
        int result = run();
        system("pause");   // Keep terminal open (for Windows)
        return result;
    }
    catch (...) {
        char message[200];
        strerror_s(message, sizeof(message), errno);
        std::cerr << message << std::endl;
        system("pause");
        return 1;
    }
}
