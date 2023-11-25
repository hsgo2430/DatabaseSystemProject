#pragma once
#include <map>
#include <fstream>

class WordCountMapper : public IMapper {
public:
    void map(const std::string& key, const std::string& value) override;
    ConcurrentQueue<std::pair<std::string, std::string>>& getQueue();
private:
    ConcurrentQueue<std::pair<std::string, std::string>> concurrentQueue;
};

class WordCountReducer : public IReducer {
public:
    WordCountReducer(const std::string& filename);
    ~WordCountReducer();
    void reduce(ConcurrentQueue<std::pair<std::string, std::string>>& concurrentQueue) override;
    void writeResults();
private:
    std::ofstream outputFile;
    std::map<std::string, int> results;
};

#include "WordCount.cpp"
