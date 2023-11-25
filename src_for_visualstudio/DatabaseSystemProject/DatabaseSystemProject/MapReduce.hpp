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
    virtual void map(const std::string& key, const std::string& value) = 0;
};

class IReducer {
public:
    virtual ~IReducer() {}
    virtual void reduce(ConcurrentQueue<std::pair<std::string, std::string>>& concurrentQueue) = 0;
};

#include "MapReduce.cpp"
