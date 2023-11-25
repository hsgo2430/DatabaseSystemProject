#include "MapReduce.h"

template <typename T> void ConcurrentQueue::push(T element) {
	std::lock_guard<std::mutex> lock(mtx);
	queue.push(std::move(element));
}

bool ConcurrentQueue::try_pop(T& popped_value) {
    std::lock_guard<std::mutex> lock(mtx);
    if (queue.empty()) {
        return false;
    }
    popped_value = queue.front();
    queue.pop();
    return true;
}