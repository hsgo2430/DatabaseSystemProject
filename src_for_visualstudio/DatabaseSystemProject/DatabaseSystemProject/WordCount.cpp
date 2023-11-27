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

std::vector<std::string> getDataFromFile(const std::string& filename) {
    std::ifstream file(filename);
    std::vector<std::string> data;
    std::string line;

    while (std::getline(file, line)) {
        data.push_back(line);
    }
    file.close();

    return data;
}

std::string wideStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::mutex m_progress;

void countWords(const std::wstring& filename, bool inplace, int m) {
    WordCountMapper mapper(inplace);
    std::cout << std::endl;
    std::wcout << L"Reading file: " << filename << std::endl;
    auto data = getDataFromFile(wideStringToString(filename));
    long t_num = static_cast<double>(data.size());
    std::vector<std::thread> mapThreads;

    int prev_progress = 0;
    long done_num = 0;
    std::cout << "Mapping words..." << "    ";

    for (size_t i = 0; i < data.size(); ++i) {
        mapThreads.emplace_back([line = data[i], i, &mapper, &done_num, t_num, &prev_progress]() {
            mapper.map(std::to_string(i), line);

            int progress = static_cast<int>(static_cast<double>(++done_num) / t_num * 100);
            if (progress > prev_progress && progress % 5 == 0) {
                m_progress.lock();
                std::cout << "\b\b\b\b    \b\b\b\b";
                printf("%3d%%", progress);
                m_progress.unlock();
            }
            prev_progress = progress;
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
    size_t runs = data.size();  // run size
    size_t outs = runs / m;     // output size
    std::string i_key = std::to_string(step);

    while (runs > 1) {
        ++step;
        std::string o_key = std::to_string(step);
        size_t mem = Profiler::getInstance()->getAllocatedMemory();
        std::cout << '[' << step << "]\t" << m << "-way merge : " << runs << " runs, " << outs << " outs, " << mem << " bytes" << std::endl;

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