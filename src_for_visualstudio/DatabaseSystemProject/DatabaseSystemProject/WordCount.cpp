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

void countWords(const std::wstring& filename, bool inplace, int m) {
    WordCountMapper mapper(inplace);
    std::cout << std::endl;
    std::wcout << L"Reading file: " << filename << std::endl;
    auto lines = getFileLines(wideStringToString(filename));
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
    int r = -1;  // input run index
    int o = -1;  // output run index
    int step = 0;
    size_t runs = lines.size();  // run size
    size_t outs = runs / m;     // output size
    std::string i_key = std::to_string(step);

    while (runs > 1) {
        ++step;
        std::string o_key = std::to_string(step);
        size_t mem = Profiler::getInstance()->getAllocatedMemory();
        std::cout << '[' << step << "]\t" << m << "-way merge : " << runs << " runs, " << outs << " outs, " << toBytesFormat(mem) << std::endl;

        for (int i = 0; i < outs; i++) {
            threads.emplace_back([&]() {
                std::vector<std::string> files;

                for (int j = 0; j < m; j++) {
                    ++r;
                    std::string r_key = std::to_string(r);
                    std::string rfilename = "run_" + i_key + "_" + r_key + ".txt";
                    std::ifstream file(rfilename);

                    if (file.is_open()) {
                        files.push_back(rfilename);
                    }
                    else {
                        --r;
                        break;
                    }
                }

                ++o;
                std::string r_key = std::to_string(o);
                std::string output;

                if (outs > 1) {
                    output = "run_" + o_key + "_" + r_key + ".txt";
                }
                else {
                    output = "output.txt";
                }

                WordCountReducer reducer(output);
                reducer.reduce(files);
                reducer.writeRun();
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        threads.clear();
        runs = o + 1;
        outs = runs / m;
        i_key = o_key;
        r = -1;
        o = -1;
    }

    std::cout << "Merge complete." << std::endl;
}