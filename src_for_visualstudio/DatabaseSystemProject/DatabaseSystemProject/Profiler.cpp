#include <iostream>
#include <cstdlib>
#include "Profiler.hpp"

Profiler* Profiler::instance = NULL;

Profiler* Profiler::getInstance() {
    if (instance == NULL) {
        instance = new Profiler();
    }
    return instance;
}

Profiler::Profiler() {
    trackNew = false;
    trackDel = false;
    alloc = 0;
    maxAlloc = 0;
}

void Profiler::start() {
    std::atexit([]() {
        Profiler::getInstance()->stop();
    });
    trackNew = true;
    trackDel = true;
}

void Profiler::stop() {
    trackNew = false;
    trackDel = false;
}

size_t Profiler::getPeakMemory() {
    return maxAlloc;
}

void* operator new(size_t size) {
    void* ptr = malloc(size);

    // Check if profiler is available (prevents endless loop)
    if (Profiler::instance != NULL) {
        auto* p = Profiler::getInstance();

        if (p->trackNew) {
            auto key = reinterpret_cast<std::uintptr_t>(ptr);
            p->trackNew = false;
            p->pmap[key] = size;
            p->trackNew = true;
            p->alloc += size;
            p->maxAlloc = std::max(p->alloc, p->maxAlloc);

            // verbose log
            //std::cout << "malloc size " << size << std::endl;
        }
    }

    return ptr;
}

void operator delete(void* ptr) {
    auto* p = Profiler::getInstance();
    auto key = reinterpret_cast<std::uintptr_t>(ptr);
    free(ptr);

    if (!p->trackDel) return;

    auto* map = &(p->pmap);
    auto iter = map->find(key);

    if (iter != map->end()) {
        size_t size = map->at(key);
        map->erase(key);
        p->alloc -= size;

        // verbose log
        //std::cout << "free size " << size << std::endl;
    }
}
