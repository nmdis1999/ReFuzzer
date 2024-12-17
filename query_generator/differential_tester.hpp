#ifndef DIFFERENTIAL_TESTER_HPP
#define DIFFERENTIAL_TESTER_HPP

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <array>
#include <sstream>
#include <cstdio>

namespace fs = std::filesystem;

class DifferentialTester {
private:
    struct CompilerConfig {
        std::string name;
        std::string command;
    };

    const std::vector<CompilerConfig> configs = {
        {"gcc-O0", "gcc -O0"},
        {"clang-O0", "clang -O0"},
        {"clang-O1", "clang -O1"},
        {"clang-O2", "clang -O2"},
        {"clang-O3", "clang -O3"}
    };

    bool executeCommand(const std::string& command, std::string& output) {
        std::array<char, 128> buffer;
        output.clear();

        FILE* pipe = popen((command + " 2>&1").c_str(), "r");
        if (!pipe) {
            std::cerr << "Error executing command" << std::endl;
            return false;
        }

        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            output += buffer.data();
        }

        return pclose(pipe) == 0;
    }

    void logDifferentCommands(const std::string& sourceFile,
                            const std::map<std::string, std::string>& outputs) {
        std::ofstream logFile("commands_" + fs::path(sourceFile).stem().string() + ".txt",
                            std::ios::app);
        if (!logFile.is_open()) {
            std::cerr << "Failed to open log file" << std::endl;
            return;
        }

        logFile << "\n=== Differential Testing Results for " << sourceFile << " ===\n";

        std::string baseOutput = outputs.begin()->second;
        std::string baseCommand = outputs.begin()->first;
        logFile << "Base command (" << baseCommand << ") output:\n" << baseOutput << "\n";

        for (const auto& [command, output] : outputs) {
            if (command == baseCommand) continue;
            if (output != baseOutput) {
                logFile << "\nDifferent output from command: " << command << "\n";
                logFile << "Output:\n" << output << "\n";
            }
        }

        logFile << "=====================================\n";
        logFile.close();
    }

public:
    void processSourceFile(const std::string& sourcePath) {
        if (!fs::exists(sourcePath)) {
            std::cerr << "Source file not found: " << sourcePath << std::endl;
            return;
        }

        std::string executableBase = "../test/" + fs::path(sourcePath).stem().string();
        std::map<std::string, std::string> outputs;
        bool foundDifference = false;

        for (const auto& config : configs) {
            std::string executablePath = executableBase + "_" + config.name;
            std::string compileCommand = config.command + " " + sourcePath + " -o " + executablePath;

            std::string compileOutput;
            if (!executeCommand(compileCommand, compileOutput)) {
                std::cout << "Compilation failed for " << config.name << ": " << compileOutput << std::endl;
                continue;
            }

            std::string runOutput;
            if (!executeCommand(executablePath, runOutput)) {
                std::cout << "Execution failed for " << config.name << ": " << runOutput << std::endl;
                continue;
            }

            outputs[config.command] = runOutput;

            if (outputs.size() > 1 && runOutput != outputs.begin()->second) {
                foundDifference = true;
            }
        }

        if (foundDifference) {
            std::cout << "Found differences in outputs for " << sourcePath << std::endl;
            logDifferentCommands(sourcePath, outputs);
        } else {
            std::cout << "All compiler configurations produced the same output for "
                     << sourcePath << std::endl;
        }
    }

    void processAllFiles() {
        try {
            for (const auto& entry : fs::directory_iterator("../correct_code")) {
                if (entry.path().extension() == ".c") {
                    std::cout << "Processing: " << entry.path().string() << std::endl;
                    processSourceFile(entry.path().string());
                }
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Filesystem error: " << e.what() << std::endl;
        }
    }
};

#endif
