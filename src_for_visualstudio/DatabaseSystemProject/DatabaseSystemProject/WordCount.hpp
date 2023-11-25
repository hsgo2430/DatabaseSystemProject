#pragma once
#include <map>
#include <string>
#include <fstream>

class WordCountMapper : public IMapper {
public:
    void map(const std::string& key, const std::string& value) override;
    ConcurrentQueue<std::pair<std::string, int>>& getQueue();
private:
    ConcurrentQueue<std::pair<std::string, int>> concurrentQueue;
};

class WordCountReducer : public IReducer {
public:
    WordCountReducer(const std::string& filename);
    ~WordCountReducer();

   /**
    * Reduce runs in-place by internal sort.
    * @param concurrentQueue contents of runs
    */
    void reduce(ConcurrentQueue<std::pair<std::string, int>>& concurrentQueue) override;

   /**
    * Reduce intermediate runs by m-way balanced merge.
    * @param runs files to merge
    */
    void reduce(std::vector<std::string> runs);

    // Write merge results to another run file.
    void writeRun();
private:
    std::ofstream outputFile;
    std::map<std::string, int> results;
};

#include "WordCount.cpp"
