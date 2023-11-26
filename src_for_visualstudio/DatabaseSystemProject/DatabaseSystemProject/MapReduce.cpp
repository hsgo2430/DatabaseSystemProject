#include "MapReduce.hpp"

template <typename T>
void ConcurrentQueue<T>::push(T element) {
    std::lock_guard<std::mutex> lock(mtx);
    queue.push(std::move(element));
}

template <typename T>
bool ConcurrentQueue<T>::try_pop(T& popped_value) {
    std::lock_guard<std::mutex> lock(mtx);
    if (queue.empty()) {
        return false;
    }
    popped_value = queue.front();
    queue.pop();
    return true;
}
