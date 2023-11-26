#include <iostream>
#include <cstdlib>
#include <windows.h>
#include "MapReduce.hpp"
#include "WordCount.hpp"

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

int main() {
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

    bool inplace = false;    // in-place vs external-sort
    int m = 2;               // m-way balanced merge
    auto start = std::chrono::steady_clock::now();

    countWords(inputFilename, inplace, m);

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Elapsed time: " << duration.count() << " ms" << std::endl;
    system("pause");
    return 0;
}
