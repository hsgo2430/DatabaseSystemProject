#pragma once
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