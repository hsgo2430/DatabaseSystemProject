#pragma once
#include <map>
#include <deque>
#include <string>
#include <fstream>

class WordCountMapper : public IMapper {
public:
    WordCountMapper(bool inplace);
    void map(const std::string& key, std::string& value) override;
    ConcurrentQueue<std::pair<std::string, int>>& getGlobalQueue();
    //ConcurrentMap<std::string, int>& getGlobalMap();
private:
    bool inplace;
    ConcurrentQueue<std::pair<std::string, int>> globalQueue;
};

class WordCountReducer : public IReducer {
public:
    WordCountReducer(const std::string& filename);
    ~WordCountReducer();
    void reduce(ConcurrentQueue<std::pair<std::string, int>>& queue) override;
    void reduce(std::vector<std::string> runs) override;
    void writeRun();
private:
    std::ofstream outputFile;
    std::map<std::string, int> results;
};

std::string getRunFilename(const std::string& step, const std::string& index);

/**
 * Count words in the given lines of text.
 * @param lines deque storing every line of text
 * @param inplace tells if external sorting algorithm should be applied to reduce tasks
 * @param m (optional) m-way balanced merge is applied only if inplace = true
 * @param batch (optional) number of lines to be mapped per each thread
 */
void countWords(std::deque<std::string>& lines, bool inplace, int m = 2, int batch = 1);

#include "WordCount.cpp"
