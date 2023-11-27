#pragma once
#include <map>
#include <cstdint>
#include <chrono>

namespace chrono = std::chrono;
typedef long long time_t;

/* Benchmark result */
struct Benchmark {
	// Duration between start and finish in milliseconds
	time_t elapsedTime;
	
	// Maximum size of memory allocated
	size_t maxMemory;

	// Average size of memory allocated
	size_t avgMemory;
};

/* Profiler lets you measure the performance of any task. */
class Profiler {
public:
	/**
	 * Start benchmarking
	 * @return the profiler instance
	 */
	static Profiler& start();

	// Access the instance
	static Profiler* getInstance();

	// Finish benchmarking
	Benchmark finish();

	// Reset metrics
	void reset();

	// Get current size of memory allocated
	size_t getAllocatedMemory();

	// Delete copy constructor
	Profiler(const Profiler& obj) = delete;

	// Allow overloading function to access private members
	friend void* _new(size_t size);

	// Allow overloading function to access private members
	friend void _delete(void* ptr);

private:
	// Constructor
	Profiler();

	// Singleton instance
	static Profiler* instance;

	// Is this profiler running?
	bool running;

	// Should malloc be tracked?
	bool trackNew;

	// Should free be tracked?
	bool trackDel;

	// Current memory allocation
	size_t alloc;

	// Peak memory allocation
	size_t maxAlloc;

	// Aggregated samples of memory allocation
	size_t aggAlloc;

	// Number of samples
	long steps;

	// Aggregated time consumed to record samples
	time_t lagTime;

	// Memory size tracker
	std::map<std::uintptr_t, size_t> pmap;

	// The point in time when benchmark is started
	chrono::time_point<chrono::steady_clock> startPoint;
};
