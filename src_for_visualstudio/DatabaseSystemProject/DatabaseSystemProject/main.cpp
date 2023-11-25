#include <iostream>
#include <cstdlib>
#include <map>
#include <vector>
#include <sstream>
#include <thread>
#include <mutex>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <queue>
#include <windows.h>
#include "MapReduce.h"
#include "WordCount.h"

std::vector<std::string> GetDataFromFile(const std::string& filename) {
    std::ifstream file(filename);
    std::vector<std::string> data;
    std::string line;
    while (std::getline(file, line)) {
        data.push_back(line);
    }
    file.close();
    return data;
}

bool GetFileNameFromDialog(std::wstring& outFilename) {
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

std::string WideStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

int main() {
    WordCountMapper mapper;
    std::wstring inputFilename;

    auto locale = std::locale(".utf-8");
    std::locale::global(locale);    // Set global locale as UTF-8
    std::wcout.imbue(locale);       // Apply UTF-8 to wcout
    std::wcin.imbue(locale);        // Apply UTF-8 to wcin
    SetConsoleOutputCP(CP_UTF8);    // Apply UTF-8 to console

    if (!GetFileNameFromDialog(inputFilename)) {
        std::cout << "No file was selected." << std::endl;
        system("pause");
        return 1;
    }

    std::wcout << L"Reading file: " << inputFilename << std::endl;
    auto data = GetDataFromFile(WideStringToString(inputFilename));
    auto start = std::chrono::steady_clock::now();

    std::vector<std::thread> mapThreads;
    for (int i = 0; i < data.size(); ++i) {
        mapThreads.emplace_back([line = data[i], i, &mapper]() {
            mapper.Map(std::to_string(i), line);
        });
    }

    for (auto& thread : mapThreads) {
        thread.join();
    }

    WordCountReducer reducer("output.txt");
    reducer.Reduce(mapper.getQueue());
    reducer.WriteResults();

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Elapsed time: " << duration.count() << " ms" << std::endl;

    system("pause");
    return 0;
}
