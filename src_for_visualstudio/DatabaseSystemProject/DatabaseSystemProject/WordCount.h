#pragma once
#include "MapReduce.h"

class WordCountMapper : public IMapper {
public:
    void Map(const std::string& key, const std::string& value) override {
        std::string str = value;
        std::transform(str.cbegin(), str.cend(), str.begin(), [](unsigned char c) {
            // Preprocess input keywords
            return std::isalpha(c) ? std::tolower(c) : ' ';
            });
        std::istringstream stream(str);
        std::string word;

        // A file storing intermediate mapping result
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

ConcurrentQueue<std::pair<std::string, std::string>> WordCountMapper::concurrentQueue;
