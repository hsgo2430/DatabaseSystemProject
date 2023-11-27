#include <iostream>
#include <cstdlib>
#include <mutex>
#include <windows.h>
#include <psapi.h>
#include "malloc.h"
#include "Profiler.hpp"

Profiler* Profiler::instance = NULL;

std::mutex m1, m2;

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
    //size_t& maxMemory = maxAlloc;
    //size_t avgMemory = (steps > 0) ? (aggAlloc / steps) : 0;
    Benchmark ret = { duration, getPeakMemory(), avgAlloc };
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

// todo support platforms other than Windows
size_t Profiler::getAllocatedMemory() {
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
    return static_cast<size_t>(info.WorkingSetSize);
}

size_t Profiler::getPeakMemory() {
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
    return static_cast<size_t>(info.PeakWorkingSetSize);
}

Profiler* Profiler::getInstance() {
    if (instance == NULL) {
        instance = new Profiler();
    }
    return instance;
}

void Profiler::updateMemory() {
    aggAlloc += getAllocatedMemory();
    avgAlloc = aggAlloc / ++steps;
}

void* _new(size_t size) {
    std::lock_guard<std::mutex> lock(m1);
    void* ptr = malloc(size);
    
    if (Profiler::instance != NULL) {
        Profiler::getInstance()->updateMemory();
    }

    return ptr;
}

void _delete(void* ptr) {
    std::lock_guard<std::mutex> lock(m1);

    if (Profiler::instance != NULL) {
        Profiler::getInstance()->updateMemory();
    }

    free(ptr);
}

void _delete(void* ptr, size_t size) {
    _delete(ptr);
}

// ----- deprecated due to inaccuracy ------
// 
//void* _new(size_t size) {
//    m1.lock();
//
//    void* ptr = std::malloc(size + sizeof(size_t));
//
//    if (!ptr) {
//        m1.unlock();
//        return 0;
//    }
//
//    // Check if profiler is available (prevents endless loop)
//    if (Profiler::instance == NULL) {
//        m1.unlock();
//        return ptr;
//    }
//
//    // Track memory size
//    auto* p = Profiler::getInstance();
//
//    if (p->trackNew) {
//        p->alloc += size;
//    }
//
//    // Append memory size at front of the pointer
//    *static_cast<size_t*>(ptr) = size;
//
//    // Adjust pointer to point the actual data
//    void* new_ptr = static_cast<char*>(ptr) + sizeof(size_t);
//
//    m1.unlock();
//    return new_ptr;
//}
//
//void _delete(void* ptr) {
//    m1.lock();
//    if (Profiler::instance == NULL) {
//        free(ptr);
//        m1.unlock();
//        return;
//    }
//
//    // Adjust pointer to point the size info
//    void* size_ptr = static_cast<char*>(ptr) - sizeof(size_t);
//
//    // Get memory size
//    size_t size = *static_cast<size_t*>(size_ptr);
//
//    auto* p = Profiler::getInstance();
//
//    if (p->trackDel) {
//        p->alloc -= size;
//    }
//
//    free(size_ptr);
//    m1.unlock();
//}
//
//void _delete(void* ptr, size_t size) {
//    m1.lock();
//
//    if (Profiler::instance == NULL) {
//        free(ptr);
//        m1.unlock();
//        return;
//    }
//
//    void* size_ptr = static_cast<char*>(ptr) - sizeof(size_t);
//    auto* p = Profiler::getInstance();
//    p->alloc -= size;
//
//    free(size_ptr);
//    m1.unlock();
//}

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
    _delete(ptr, size);
}

void operator delete[](void* ptr, std::size_t size) {
    _delete(ptr, size);
}

void operator delete(void* ptr, const std::nothrow_t& tag) {
    _delete(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t& tag) {
    _delete(ptr);
}
