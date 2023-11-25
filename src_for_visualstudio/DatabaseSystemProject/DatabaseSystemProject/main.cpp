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

template <typename T>
class ConcurrentQueue {
private:
    std::queue<T> queue;
    std::mutex mtx;
public:
    void push(T element) {
        std::lock_guard<std::mutex> lock(mtx);
        queue.push(std::move(element));
    }

    bool try_pop(T& popped_value) {
        std::lock_guard<std::mutex> lock(mtx);
        if (queue.empty()) {
            return false;
        }
        popped_value = queue.front();
        queue.pop();
        return true;
    }
};

class IMapper {
public:
    virtual ~IMapper() {}
    virtual void Map(const std::string& key, const std::string& value) = 0;
};

class IReducer {
public:
    virtual ~IReducer() {}
    virtual void Reduce(ConcurrentQueue<std::pair<std::string, std::string>>& concurrentQueue) = 0;
};

class WordCountMapper : public IMapper {
public:
    void Map(const std::string& key, const std::string& value) override {
        std::string str = value;
        std::transform(str.cbegin(), str.cend(), str.begin(), [](unsigned char c) {
            return std::isalpha(c) ? std::tolower(c) : ' ';
        });
        std::istringstream stream(str);
        std::string word;

        // 맵퍼 작업 중간값 저장
        std::string filename = "map_output_" + key + ".txt";
        std::ofstream outputFile(filename);

        while (stream >> word) {
            std::pair<std::string, std::string> wordCount(word, "1");
            concurrentQueue.push(wordCount);

            outputFile << "<" << word << ", 1>\n";
        }

        outputFile.close();
    }

    static ConcurrentQueue<std::pair<std::string, std::string>>& getQueue() {
        return concurrentQueue;
    }

private:
    static ConcurrentQueue<std::pair<std::string, std::string>> concurrentQueue;
};

class WordCountReducer : public IReducer {
public:
    WordCountReducer(const std::string& filename) : outputFile(filename) {}

    void Reduce(ConcurrentQueue<std::pair<std::string, std::string>>& concurrentQueue) override {
        std::pair<std::string, std::string> wordCount;
        while (concurrentQueue.try_pop(wordCount)) {
            ++results[wordCount.first];
        }
    }

    void WriteResults() {
        std::vector<std::pair<std::string, int>> resultsVec(results.begin(), results.end());
        std::sort(resultsVec.begin(), resultsVec.end(), [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
            return a.second > b.second;
            });

        for (const auto& pair : resultsVec) {
            outputFile << pair.first << ": " << pair.second << std::endl;
        }
    }

    ~WordCountReducer() {
        outputFile.close();
    }

private:
    std::ofstream outputFile;
    std::map<std::string, int> results;
};

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

ConcurrentQueue<std::pair<std::string, std::string>> WordCountMapper::concurrentQueue;

bool GetFileNameFromDialog(std::wstring& outFilename) {
    wchar_t filename[MAX_PATH] = { 0 };
    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL; // If you have a window handle, put it here
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
    std::locale::global(std::locale(""));  // 시스템 기본 로케일을 사용하도록 설정
    std::wcout.imbue(std::locale());       // wcout에 로케일을 적용
    std::wcin.imbue(std::locale());        // wcin에 로케일을 적용

    std::wstring inputFilename;
    if (!GetFileNameFromDialog(inputFilename)) {
        std::wcout << L"No file was selected." << std::endl;
        system("pause");
        return 1;
    }

    std::cout << WideStringToString(inputFilename) << std::endl;
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
