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