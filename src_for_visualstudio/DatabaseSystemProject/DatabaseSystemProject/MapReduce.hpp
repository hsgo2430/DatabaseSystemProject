#pragma once
#include <mutex>
#include <queue>

template <typename T>
class ConcurrentQueue {
public:
    void push(T element);
    bool try_pop(T& popped_value);
private:
    std::queue<T> queue;
    std::mutex mtx;
};

class IMapper {
public:
    virtual ~IMapper() {}
    virtual void map(const std::string& key, std::string& value) = 0;
};

class IReducer {
public:
    virtual ~IReducer() {}

   /**
    * Reduce runs in-place by internal sort.
    * @param concurrentQueue contents of runs
    */
    virtual void reduce(ConcurrentQueue<std::pair<std::string, int>>& queue) = 0;

   /**
    * Reduce intermediate runs by m-way balanced merge.
    * @param runs files to merge
    */
    virtual void reduce(std::vector<std::string> runs) = 0;
};

#include "MapReduce.cpp"
