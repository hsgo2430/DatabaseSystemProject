#include <iostream>
#include <cstdlib>
#include "malloc.h"
#include <mutex>
#include "Profiler.hpp"

Profiler* Profiler::instance = NULL;

std::mutex m;

Profiler::Profiler() {
    reset();
}

Profiler& Profiler::start() {
    if (getInstance()->running) {
        throw new std::runtime_error("Profiler is already running!");
    }

    std::atexit([]() {
        getInstance()->reset();
    });

    auto* p = getInstance();
    p->running = true;
    p->trackNew = true;
    p->trackDel = true;
    p->startPoint = chrono::steady_clock::now();

    return *p;
}

Benchmark Profiler::finish() {
    if (!running) {
        throw new std::runtime_error("Profiler is not running!");
    }

    auto now = chrono::steady_clock::now();
    time_t duration = chrono::duration_cast<chrono::milliseconds>(now - startPoint).count();
    size_t& maxMemory = maxAlloc;
    size_t avgMemory = (steps > 0) ? (aggAlloc / steps) : 0;
    Benchmark ret = { duration, maxMemory, avgMemory };
    reset();

    return ret;
}

void Profiler::reset() {
    running = false;
    trackNew = false;
    trackDel = false;
    alloc = 0;
    maxAlloc = 0;
    aggAlloc = 0;
    steps = 0;
    lagTime = 0;
}

size_t Profiler::getAllocatedMemory() {
    return alloc;
}

Profiler* Profiler::getInstance() {
    if (instance == NULL) {
        instance = new Profiler();
    }
    return instance;
}

void* _new(size_t size) {
    void* ptr = malloc(size);

    // Check if profiler is available (prevents endless loop)
    if (Profiler::instance == NULL) {
        return ptr;
    }

    auto* p = Profiler::getInstance();

    if (p->trackNew) {
        auto key = reinterpret_cast<std::uintptr_t>(ptr);

        m.lock();
        p->trackNew = false;
        p->pmap[key] = size;  // calls new operator
        p->trackNew = true;
        m.unlock();

        p->alloc += size;
        p->aggAlloc += p->alloc;
        p->maxAlloc = std::max(p->alloc, p->maxAlloc);
        p->steps += 1;
    }

    return ptr;
}

void _delete(void* ptr) {
    auto* p = Profiler::getInstance();
    auto key = reinterpret_cast<std::uintptr_t>(ptr);
    free(ptr);

    if (!p->trackDel) {
        return;
    }

    auto* map = &(p->pmap);

    m.lock();
    p->trackDel = false;
    auto iter = map->find(key);

    if (iter == map->end()) {  // if key not found
        p->trackDel = true;
        m.unlock();
        return;
    }

    size_t size = map->at(key);
    map->erase(key);  // calls delete operator
    p->trackDel = true;
    m.unlock();

    p->alloc -= size;
    p->aggAlloc += p->alloc;
    p->steps += 1;
}

// ----- Dynamic allocation overloading -----

void* operator new(size_t size) {
    return _new(size);
}

void* operator new[](size_t size) {
    return _new(size);
}

void* operator new(size_t size, const std::nothrow_t& tag) {
    return _new(size);
}

void* operator new[](size_t size, const std::nothrow_t& tag) {
    return _new(size);
}

void operator delete(void* ptr) {
    _delete(ptr);
}

void operator delete[](void* ptr) {
    _delete(ptr);
}

void operator delete(void* ptr, std::size_t size) {
    _delete(ptr);
}

void operator delete[](void* ptr, std::size_t size) {
    _delete(ptr);
}

void operator delete(void* ptr, const std::nothrow_t& tag) {
    _delete(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t& tag) {
    _delete(ptr);
}
