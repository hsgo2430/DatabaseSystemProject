#include <vector>
#include <thread>
#include <string>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>
#include <filesystem>
#include <cerrno>
#include <unordered_map>
#include "Profiler.hpp"
#include "WordCount.hpp"
#include "Utility.hpp"

namespace fs = std::filesystem;

WordCountMapper::WordCountMapper(bool inplace) {
    WordCountMapper::inplace = inplace;
}

void WordCountMapper::map(const std::string& key, std::string& value) {
    static std::mutex mtx;

    std::transform(value.cbegin(), value.cend(), value.begin(), [](unsigned char c) {
        // Preprocess input keywords
        return std::isalpha(c) ? std::tolower(c) : ' ';
    });

    std::istringstream stream(value);
    std::string word;

    if (inplace) {
        while (stream >> word) {
            std::pair<std::string, int> pair(word, 1);
            globalQueue.push(pair);
        }
        return;
    }

    std::unordered_map<std::string, int> map;

    while (stream >> word) {
        ++map[word];
    }

    // A file storing intermediate mapping result
    std::string filename = getRunFilename(std::to_string(0), key);
    std::ofstream outputFile;
    std::lock_guard<std::mutex> lock(mtx);
    
    writeFile(outputFile, filename, std::ios::app | std::ios::out);

    //while (!open) {
    //    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    //    mtx.lock();
    //    open = writeFile(outputFile, filename, std::ios::app | std::ios::out);
    //    std::cout << "Attempting to re-open file: " << filename << std::endl;
    //    mtx.unlock();
    //}

    for (auto pair : map) {
        outputFile << pair.first << ": " << pair.second << '\n';
    }

    closeFile(outputFile);
}

ConcurrentQueue<std::pair<std::string, int>>& WordCountMapper::getGlobalQueue() {
    return globalQueue;
}

WordCountReducer::WordCountReducer(const std::string& filename) {
    outputFile.open(filename);
    outputFile.exceptions(std::ios::failbit | std::ifstream::badbit);
}

WordCountReducer::~WordCountReducer() {
    outputFile.close();
}

void WordCountReducer::reduce(ConcurrentQueue<std::pair<std::string, int>>& queue) {
    std::pair<std::string, int> pair;

    while (queue.try_pop(pair)) {
        results[pair.first] += pair.second;
    }
}

void WordCountReducer::reduce(std::vector<std::string> runs) {
    static std::mutex mtx_file;
    static std::mutex mtx_map;

    for (std::string filename : runs) {
        std::ifstream file;
        std::string line;

        mtx_file.lock();
        file.open(filename);
        mtx_file.unlock();

        while (true) {
            mtx_file.lock();
            bool eol = !std::getline(file, line);
            mtx_file.unlock();

            if (eol) break;

            std::string token(": ");
            auto i = line.find(token);
            std::string key = line.substr(0, i);
            int value = std::stoi(line.substr(i + token.length()));

            mtx_map.lock();
            results[key] += value;
            mtx_map.unlock();
        }

        mtx_file.lock();
        file.close();
        mtx_file.unlock();
    }
}

void WordCountReducer::writeRun() {
    static std::mutex mtx;
    std::vector<std::pair<std::string, int>> resultsVec(results.begin(), results.end());
    std::sort(resultsVec.begin(), resultsVec.end(), [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
        return a.second > b.second;
    });

    std::lock_guard<std::mutex> lock(mtx);

    for (const auto& pair : resultsVec) {
        outputFile << pair.first << ": " << pair.second << std::endl;
    }
}

std::string getRunFilename(const std::string& step, const std::string& index) {
    std::string filename = "tmp/run_" + step + "_" + index + ".txt";
    return filename;
}

void mapRoutine(long maps, int batch, long& b, std::deque<std::string>& lines, WordCountMapper &mapper) {
    static std::mutex mtx_lines;
    static std::mutex mtx_counters;
    std::string key;

    while (true) {
        if (b >= maps) break;

        mtx_counters.lock();
        key = std::to_string(b);
        ++b;
        printProgress(b, maps, false);
        mtx_counters.unlock();

        std::string filename = getRunFilename(std::to_string(0), key);
        std::string value;

        // ensure that new file doesn't overlap with existing one
        remove(filename.c_str());

        // combine lines into single string
        for (int j = 0; j < batch; ++j) {
            mtx_lines.lock();
            if (lines.empty()) {
                mtx_lines.unlock();
                break;
            }
            value.append(lines.front()).append(" ");
            lines.pop_front();
            mtx_lines.unlock();
            //value.append(lines[i + j]).append(" ");
        }

        // map lines in batch
        mapper.map(key, value);
    }
}

void mergeRoutine(int m, size_t output, size_t &mx, int &in, int &out, const std::string &i_key, const std::string &o_key) {
    static std::mutex mtx_mx;
    static std::mutex mtx_in;
    static std::mutex mtx_out;
    static std::mutex mtx_file;
    
    while (true) {
        std::vector<std::string> files;
        bool complete;

        mtx_mx.lock();
        complete = (mx < 1);
        mx = complete ? 0 : --mx;
        mtx_mx.unlock();

        if (complete) break;

        for (int j = 0; j < m; j++) {
            std::string r_key = std::to_string(in + 1);
            std::string filename = getRunFilename(i_key, r_key);

            mtx_file.lock();
            bool fileExists = fs::exists(filename);
            mtx_file.unlock();

            mtx_in.lock();
            ++in;
            mtx_in.unlock();

            if (fileExists) {
                files.push_back(filename);
            }
            else {
                throw std::runtime_error("File not exist: " + filename);
            }
        }

        mtx_out.lock();
        ++out;
        mtx_out.unlock();

        std::string r_key = std::to_string(out);
        std::string filename;

        if (output > 1) {
            filename = getRunFilename(o_key, r_key);
        }
        else {
            filename = "output.txt";
        }

        WordCountReducer reducer(filename);
        reducer.reduce(files);
        reducer.writeRun();
    }
}

void countWords(std::deque<std::string>& lines, bool inplace, int m, int batch) {
    WordCountMapper mapper(inplace);
    long maps = static_cast<long>(ceil((double) lines.size() / batch)); // number of mappings required
    long b = 0;        // batch index
    //long counter = 0;  // number of lines processed
    std::vector<std::thread> mapThreads;
    std::mutex mtx;

    // create tmp folder if not exists
    fs::path tmpPath = "tmp";
    fs::create_directory(tmpPath);

    std::cout << "Mapping words...";
    printProgress(0, maps, true);

    //for (int i = 0; (i + batch - 1) < size; i += batch) {
    int core = std::thread::hardware_concurrency();
    core = (core > 4) ? core : 4;

    for (int i = 0; i < core; ++i) {
        mapThreads.emplace_back([maps, batch, &b, &lines, &mapper]() {
            try {
                mapRoutine(maps, batch, b, lines, mapper);
            }
            catch (const std::exception& e) {
                std::cout << e.what() << std::endl;
            }
            catch (...) {
                char message[200];
                strerror_s(message, sizeof(message), errno);
                std::cerr << "Error: " << message << std::endl;
            }
        });
    }

    for (auto& thread : mapThreads) {
        thread.join();
    }

    // last batch is always incomplete
    // if size is not divisible by batch
    //if (size % batch > 0) {
    //    for (int i = size - (size % batch); i < size; ++i) {
    //        mapper.map(std::to_string(b), lines.front());
    //        lines.pop_front();
    //        printProgress(i + 1, size, false);
    //    }
    //}

    std::cout << std::endl;

    if (inplace) {
        WordCountReducer reducer("output.txt");
        reducer.reduce(mapper.getGlobalQueue());
        reducer.writeRun();
        std::cout << "Reduce complete." << std::endl;
        return;
    }

    // m-way balanced merge
    std::vector<std::thread> threads;
    int in = -1;   // input run index
    int out = -1;  // output run index
    int step = 0;  // phase of merge
    size_t runs = b;             // number of input
    size_t mx = runs / m;        // number of runs to be merged
    size_t rem = runs % m;       // number of runs remaining (not to be merged)
    size_t output = mx + rem;    // number of output
    std::string i_key = std::to_string(step);

    // repeat merge until we get single run
    while (runs > 1) {
        // if nothing can be merged because m is too high
        if (mx < 1) {
            m = runs;
            mx = runs / m;
            rem = runs % m;
            output = mx + rem;
            continue;
        }

        ++step;
        std::string o_key = std::to_string(step);
        size_t mem = Profiler::getInstance()->getAllocatedMemory();
        std::cout << '[' << step << "] " << m << "-way merge / "
            << runs << " runs, " << output << " output, " << toBytesFormat(mem) << std::endl;

        // merge runs in parallel tasks
        //for (int i = 0; i < mx; ++i) {
        for (int i = 0; i < core; ++i) {
            threads.emplace_back([m, output, &mx, &in, &out, &i_key, &o_key]() {
                try {
                    mergeRoutine(m, output, mx, in, out, i_key, o_key);
                }
                catch (const std::ios_base::failure& e) {
                    std::cout << e.what() << '\n';
                }
                catch (const std::exception& e) {
                    std::cout << e.what() << std::endl;
                }
                catch (...) {
                    char message[200];
                    strerror_s(message, sizeof(message), errno);
                    std::cout << "Error: " << message << std::endl;
                    return;
                }
            });
        }

        // wait for parallel tasks to complete
        for (auto& thread : threads) {
            thread.join();
        }

        // copy runs remaining
        for (int i = 0; i < rem; ++i) {
            ++in;
            std::string r_key = std::to_string(in);
            std::string i_filename = getRunFilename(i_key, r_key);
            std::ifstream i_file(i_filename);

            if (!i_file.is_open()) {
                throw std::system_error(EFAULT, std::iostream_category(), "Failed to open: " + i_filename);
            }

            ++out;
            r_key = std::to_string(out);
            std::string o_filename = getRunFilename(o_key, r_key);
            std::ofstream o_file(o_filename);
            std::string line;

            if (!o_file.is_open()) {
                i_file.close();
                throw std::system_error(EFAULT, std::iostream_category(), "Failed to open: " + o_filename);
            }

            while (std::getline(i_file, line)) {
                o_file << line << std::endl;
            }

            i_file.close();
            o_file.close();
        }

        // reset and prepare for next step
        threads.clear();
        runs = out + 1;
        mx = runs / m;
        rem = runs % m;
        output = mx + rem;
        i_key = o_key;
        in = -1;
        out = -1;
    }

    std::cout << "Merge complete." << std::endl;
}
