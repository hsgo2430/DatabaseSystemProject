#include <vector>
#include <string>

std::vector<std::string> getFileLines(const std::wstring& filename);

// deprecated
std::string wideStringToString(const std::wstring& wstr);

void printProgress(long target, bool init);

std::string toBytesFormat(long bytes);

std::string toTimeFormat(long milliseconds);
