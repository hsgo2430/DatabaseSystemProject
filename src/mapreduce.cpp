#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <thread>
#include <mutex>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <queue>

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

        //맵퍼 작업 중간값 저장
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

int main() {
    WordCountMapper mapper;
    auto data = GetDataFromFile("input.txt");
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

    return 0;
}
