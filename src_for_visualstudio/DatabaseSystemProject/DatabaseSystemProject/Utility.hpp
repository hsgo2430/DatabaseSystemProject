#include <deque>
#include <string>

std::deque<std::string> getFileLines(const std::wstring& filename);

// deprecated
std::string wideStringToString(const std::wstring& wstr);

void printProgress(long counter, long target, bool init);

std::string toBytesFormat(size_t bytes);

std::string toTimeFormat(time_t milliseconds);

void writeFile(std::ofstream &file, const std::string& filename, std::ios::openmode mode);

void closeFile(std::ofstream &file);
