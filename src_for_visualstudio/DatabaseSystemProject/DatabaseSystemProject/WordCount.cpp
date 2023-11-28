#include <vector>
#include <thread>
#include <string>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>
#include "Profiler.hpp"
#include "WordCount.hpp"
#include "Utility.hpp"

WordCountMapper::WordCountMapper(bool inplace) {
    WordCountMapper::inplace = inplace;
}

void WordCountMapper::map(const std::string& key, const std::string& value) {
    std::string str = value;
    std::transform(str.cbegin(), str.cend(), str.begin(), [](unsigned char c) {
        // Preprocess input keywords
        return std::isalpha(c) ? std::tolower(c) : ' ';
    });

    std::vector<std::pair<std::string, int>> pairs;
    std::istringstream stream(str);
    std::string word;

    while (stream >> word) {
        std::pair<std::string, int> wordCount(word, 1);
        pairs.push_back(wordCount);
        concurrentQueue.push(wordCount);
    }

    if (!inplace) {
        // A file storing intermediate mapping result
        std::string filename = "run_0_" + key + ".txt";
        std::ofstream outputFile(filename);

        for (auto pair : pairs) {
            outputFile << pair.first << ": " << pair.second << '\n';
        }
        outputFile.close();
    }
}

ConcurrentQueue<std::pair<std::string, int>>& WordCountMapper::getQueue() {
    return concurrentQueue;
}

WordCountReducer::WordCountReducer(const std::string& filename) : outputFile(filename) {}

WordCountReducer::~WordCountReducer() {
    outputFile.close();
}

void WordCountReducer::reduce(ConcurrentQueue<std::pair<std::string, int>>& concurrentQueue) {
    std::pair<std::string, int> wordCount;
    while (concurrentQueue.try_pop(wordCount)) {
        results[wordCount.first] += wordCount.second;
    }
}

void WordCountReducer::reduce(std::vector<std::string> runs) {
    for (std::string filename : runs) {
        std::ifstream file(filename);
        std::string line;

        while (std::getline(file, line)) {
            std::string token(": ");
            auto i = line.find(token);
            std::string key = line.substr(0, i);
            int value = std::stoi(line.substr(i + token.length()));
            results[key] += value;
        }
        file.close();
    }
}

void WordCountReducer::writeRun() {
    std::vector<std::pair<std::string, int>> resultsVec(results.begin(), results.end());
    std::sort(resultsVec.begin(), resultsVec.end(), [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
        return a.second > b.second;
    });

    for (const auto& pair : resultsVec) {
        outputFile << pair.first << ": " << pair.second << std::endl;
    }
}

void countWords(std::vector<std::string>& lines, bool inplace, int m) {
    WordCountMapper mapper(inplace);
    std::vector<std::thread> mapThreads;
    long t_num = static_cast<double>(lines.size());

    std::cout << "Mapping words...";
    printProgress(t_num, true);

    for (long i = 0; i < t_num; ++i) {
        mapThreads.emplace_back([line = lines[i], i, &mapper, t_num]() {
            mapper.map(std::to_string(i), line);
            printProgress(t_num, false);
        });
    }

    for (auto& thread : mapThreads) {
        thread.join();
    }

    std::cout << std::endl;

    if (inplace) {
        WordCountReducer reducer("output.txt");
        reducer.reduce(mapper.getQueue());
        reducer.writeRun();
        std::cout << "Reduce complete." << std::endl;
        return;
    }

    // m-way balanced merge
    std::vector<std::thread> threads;
    int in = -1;  // input run index
    int out = -1;  // output run index
    int step = 0;
    size_t runs = lines.size();  // number of input
    size_t mx = runs / m;        // number of runs to be merged
    size_t rem = runs % m;       // number of runs remaining (not merged)
    size_t output = mx + rem;      // number of output
    std::string i_key = std::to_string(step);

    while (runs > 1) {
        ++step;
        std::string o_key = std::to_string(step);
        size_t mem = Profiler::getInstance()->getAllocatedMemory();
        std::cout << '[' << step << "]\t" << m << "-way merge : "
            << runs << " runs, " << output << " output, " << toBytesFormat(mem) << std::endl;

        if (mx < 1) {  // if nothing can be merged
            --step;
            --m;
            mx = runs / m;
            rem = runs % m;
            output = mx + rem;
            continue;
        }

        for (int i = 0; i < mx; ++i) {
            threads.emplace_back([&]() {
                std::vector<std::string> files;

                for (int j = 0; j < m; j++) {
                    ++in;  // todo apply mutex
                    std::string r_key = std::to_string(in);
                    std::string filename = "run_" + i_key + "_" + r_key + ".txt";
                    std::ifstream file(filename);

                    if (file.is_open()) {
                        files.push_back(filename);
                    }
                    else {
                        --in;
                        break;
                    }
                }

                ++out;  // todo apply mutex
                std::string r_key = std::to_string(out);
                std::string filename;

                if (output > 1) {
                    filename = "run_" + o_key + "_" + r_key + ".txt";
                }
                else {
                    filename = "output.txt";
                }

                WordCountReducer reducer(filename);
                reducer.reduce(files);
                reducer.writeRun();
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
            std::string i_filename = "run_" + i_key + "_" + r_key + ".txt";
            std::ifstream i_file(i_filename);

            if (!i_file.is_open()) {
                return;
            }

            ++out;
            r_key = std::to_string(out);
            std::string o_filename = "run_" + o_key + "_" + r_key + ".txt";
            std::ofstream o_file(o_filename);
            std::string line;

            if (!o_file.is_open()) {
                i_file.close();
                return;
            }

            while (std::getline(i_file, line)) {
                o_file << line << std::endl;
            }

            i_file.close();
            o_file.close();
        }

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
