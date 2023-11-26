#include <iostream>
#include <fstream>
#include <cstdlib>
#include <windows.h>
#include "MapReduce.hpp"
#include "WordCount.hpp"
#include "Profiler.hpp"

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

long getLines(const std::wstring& filename) {
    std::ifstream file(filename);
    std::string word;
    long lines = 0;

    if (file.is_open()) {
        while (file >> word) {
            ++lines;
        }
    }
    return lines;
}

int main() {
    // Start memory profiler
    auto* profiler = Profiler::getInstance();
    profiler->start();


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
        system("pause");
        return 1;
    }

    std::cout << getLines(inputFilename) << " lines of text found!" << std::endl;

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
        std::cout << "Select m-way balanced merge (default=2): ";
        std::cin.ignore(INT_MAX, '\n');
        std::getline(std::cin, str);

        try {
            m = std::stoi(str);
        }
        catch (...) {
            m = 2;
        }
    }


    // Run map-reduce application
    auto start = std::chrono::steady_clock::now();

    countWords(inputFilename, inplace, m);

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    auto memory = profiler->getPeakMemory();
    profiler->stop();

    std::cout << std::endl;
    std::cout << "Elapsed time: " << duration.count() << " ms" << std::endl;
    std::cout << "Peak memory: " << memory << " bytes" << std::endl;

    system("pause");   // Keep terminal open (for Windows)
    return 0;
}
