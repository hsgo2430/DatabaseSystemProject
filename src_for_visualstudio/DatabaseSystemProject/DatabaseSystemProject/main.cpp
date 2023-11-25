#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>
#include <thread>
#include <windows.h>
#include "MapReduce.hpp"
#include "WordCount.hpp"

std::vector<std::string> getDataFromFile(const std::string& filename) {
    std::ifstream file(filename);
    std::vector<std::string> data;
    std::string line;

    while (std::getline(file, line)) {
        data.push_back(line);
    }
    file.close();

    return data;
}

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

std::string wideStringToString(const std::wstring& wstr) {
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
    std::wcout.imbue(locale);       // Apply locale to wcout
    std::wcin.imbue(locale);        // Apply locale to wcin
    SetConsoleOutputCP(CP_UTF8);    // Apply locale to console

    if (!getFileNameFromDialog(inputFilename)) {
        std::cout << "No file was selected." << std::endl;
        system("pause");
        return 1;
    }

    std::wcout << L"Reading file: " << inputFilename << std::endl;
    auto data = getDataFromFile(wideStringToString(inputFilename));
    auto start = std::chrono::steady_clock::now();
    std::vector<std::thread> mapThreads;

    for (int i = 0; i < data.size(); ++i) {
        mapThreads.emplace_back([line = data[i], i, &mapper]() {
            mapper.map(std::to_string(i), line);
        });
    }

    for (auto& thread : mapThreads) {
        thread.join();
    }

    // Balanced merge
    std::vector<std::thread> threads;
    int m = 2;  // m-way merge
    int r = -1;  // input run index
    int o = -1;  // output run index
    size_t runs = data.size();  // run size
    size_t outs = runs / m;     // output size

    while (runs > 1) {
        // debug
        std::cout << m << "-way merge : " << runs << " runs, " << outs << " outs" << std::endl;

        for (int i = 0; i < outs; i++) {
            threads.emplace_back([&]() {
                std::vector<std::string> files;

                for (int j = 0; j < m; j++) {
                    std::string key = std::to_string(++r);
                    std::string filename = "map_output_" + key + ".txt";
                    std::ifstream file(filename);

                    if (file.is_open()) {
                        files.push_back(filename);
                    }
                    else {
                        --r;
                        break;
                    }
                }

                std::string outKey = std::to_string(++o);
                std::string output = "map_output_" + outKey + ".txt";
                WordCountReducer reducer(output);
                reducer.reduce(files);

                for (auto filename : files) {
                    std::remove(filename.c_str());
                }

                reducer.writeRun();
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        runs = o + 1;
        outs = runs / m;
    }

    // debug
    std::cout << "Merge complete." << std::endl;

    //WordCountReducer reducer("output.txt");
    //reducer.reduce(mapper.getQueue());
    //reducer.writeRun();

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Elapsed time: " << duration.count() << " ms" << std::endl;

    system("pause");
    return 0;
}
