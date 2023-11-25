#include <vector>
#include <sstream>
#include <iostream>
#include <cctype>
#include "WordCount.hpp"

void WordCountMapper::map(const std::string& key, const std::string& value) {
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

ConcurrentQueue<std::pair<std::string, std::string>>& WordCountMapper::getQueue() {
    return concurrentQueue;
}

WordCountReducer::WordCountReducer(const std::string& filename) : outputFile(filename) {}

WordCountReducer::~WordCountReducer() {
    outputFile.close();
}

void WordCountReducer::reduce(ConcurrentQueue<std::pair<std::string, std::string>>& concurrentQueue) {
    std::pair<std::string, std::string> wordCount;
    while (concurrentQueue.try_pop(wordCount)) {
        ++results[wordCount.first];
    }
}

void WordCountReducer::writeResults() {
    std::vector<std::pair<std::string, int>> resultsVec(results.begin(), results.end());
    std::sort(resultsVec.begin(), resultsVec.end(), [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
        return a.second > b.second;
        });

    for (const auto& pair : resultsVec) {
        outputFile << pair.first << ": " << pair.second << std::endl;
    }
}
