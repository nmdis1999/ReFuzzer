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

   const CompilerConfig baseConfig = {"gcc-O0", "gcc -O0"};
   const std::vector<CompilerConfig> configs = {
       {"clang15-O0", "clang-15 -O0"},
       {"clang15-O1", "clang-15 -O1"},
       {"clang15-O2", "clang-15 -O2"},
       {"clang15-O3", "clang-15 -O3"},
       {"clang16-O0", "clang-16 -O0"},
       {"clang16-O1", "clang-16 -O1"},
       {"clang16-O2", "clang-16 -O2"},
       {"clang16-O3", "clang-16 -O3"},
       {"clang17-O0", "clang-17 -O0"},
       {"clang17-O1", "clang-17 -O1"},
       {"clang17-O2", "clang-17 -O2"},
       {"clang17-O3", "clang-17 -O3"}
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

   void logDifferentCommands(const std::string& sourceFile, const std::string& baseOutput, const std::map<std::string, std::string>& outputs) {
       std::ofstream logFile("commands_" + fs::path(sourceFile).stem().string() + ".txt", std::ios::app);
       if (!logFile.is_open()) {
           std::cerr << "Failed to open log file" << std::endl;
           return;
       }
       logFile << "\n=== Differential Testing Results for " << sourceFile << " ===\n";
       logFile << "Base command (" << baseConfig.command << ") output:\n" << baseOutput << "\n";
       for (const auto& [command, output] : outputs) {
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

       std::string baseExecutablePath = executableBase + "_" + baseConfig.name;
       std::string baseCompileCommand = baseConfig.command + " " + sourcePath + " -o " + baseExecutablePath;

       std::string baseCompileOutput;
       if (!executeCommand(baseCompileCommand, baseCompileOutput)) {
           std::cout << "Base compilation failed: " << baseCompileOutput << std::endl;
           return;
       }

       std::string baseRunOutput;
       if (!executeCommand(baseExecutablePath, baseRunOutput)) {
           std::cout << "Base execution failed: " << baseRunOutput << std::endl;
           return;
       }

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

           if (runOutput != baseRunOutput) {
               foundDifference = true;
           }
       }

       if (foundDifference) {
           std::cout << "Found differences in outputs for " << sourcePath << std::endl;
           logDifferentCommands(sourcePath, baseRunOutput, outputs);
       } else {
           std::cout << "All compiler configurations matched gcc -O0 output for " << sourcePath << std::endl;
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
