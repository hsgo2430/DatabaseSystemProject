#pragma once
#include <map>
#include <string>
#include <fstream>

class WordCountMapper : public IMapper {
public:
    WordCountMapper(bool inplace);
    void map(const std::string& key, const std::string& value) override;
    ConcurrentQueue<std::pair<std::string, int>>& getQueue();
private:
    bool inplace;
    ConcurrentQueue<std::pair<std::string, int>> concurrentQueue;
};

class WordCountReducer : public IReducer {
public:
    WordCountReducer(const std::string& filename);
    ~WordCountReducer();
    void reduce(ConcurrentQueue<std::pair<std::string, int>>& concurrentQueue) override;
    void reduce(std::vector<std::string> runs) override;
    void writeRun();
private:
    std::ofstream outputFile;
    std::map<std::string, int> results;
};

/**
 * Count words in the given file.
 * @param filename name of the input file
 * @param inplace tells if external sorting algorithm should be applied to reduce tasks
 * @param m (optional) m-way balanced merge is applied only if inplace = true
 */
void countWords(const std::wstring& filename, bool inplace, int m = 2);

#include "WordCount.cpp"
