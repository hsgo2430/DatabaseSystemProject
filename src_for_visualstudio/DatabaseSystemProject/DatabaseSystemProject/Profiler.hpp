#pragma once
#include <map>
#include <cstdint>

class Profiler {
public:
   /**
	* Access the instance of this class.
	* @return a pointer to the instance
	*/
	static Profiler* getInstance();

	// Start memory profiler
	void start();

	// Stop memory profiler
	void stop();

	// Get maximum size of memory allocated
	size_t getPeakMemory();

	// Delete copy constructor
	Profiler(const Profiler& obj) = delete;

	// Allow private access to new operator
	friend void* operator new(size_t size);

	// Allow private access to delete operator
	friend void operator delete(void* ptr);

private:
	// Constructor
	Profiler();

	// Singleton instance
	static Profiler* instance;

	// Memory size tracker
	std::map<std::uintptr_t, size_t> pmap;

	// Should malloc be tracked?
	bool trackNew;

	// Should free be tracked?
	bool trackDel;

	// Current size of memory allocation
	size_t alloc;

	// Peak size of memory allocation
	size_t maxAlloc;
};
