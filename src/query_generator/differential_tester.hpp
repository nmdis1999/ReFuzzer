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
#include <regex>
#include <sys/wait.h>  

namespace fs = std::filesystem;

class DifferentialTester {
private:
   struct CompilerConfig {
       std::string name;
       std::string command;
   };

   const std::vector<CompilerConfig> configs = {
       {"gcc-O0", "gcc -O0"},
       {"gcc-O1", "gcc -O1"},
       {"gcc-O2", "gcc -O2"},
       {"gcc-O3", "gcc -O3"},
       {"clang-O0", "clang -O0"},
       {"clang-O1", "clang -O1"},
       {"clang-O2", "clang -O2"},
       {"clang-O3", "clang -O3"}
   };
   const std::vector<std::string> crashPatterns = {
       "Segmentation fault",
       "LLVM ERROR:",
       "internal compiler error",
       "compiler crashed",
       "UNREACHABLE executed",
       "PLEASE submit a bug report",
       "Assertion .* failed",
       "ICE",
       "llvm::report_fatal_error",
       "clang: error: unable to execute command",
       "gcc: internal compiler error",
       "FATAL ERROR",
       "panicked at",
       "Backend compiler crashed",
       "Illegal instruction",
       "Aborted"
   };

   bool isCompilerCrash(const std::string& output, int exitCode) {
       for (const auto& pattern : crashPatterns) {
           std::regex regex(pattern, std::regex::icase);
           if (std::regex_search(output, regex)) {
               return true;
           }
       }
       
       if (WIFSIGNALED(exitCode)) {
           return true;
       }
       
       // Special case for abnormal exit codes that might indicate crashes
       // Normal compilation errors usually exit with 1
       // Exit codes >= 128 often indicate crashes or signal termination
       if (exitCode > 1 && exitCode != 255) {  
           return true;
       }
       
       return false;
   }

   bool executeCommand(const std::string& command, std::string& output, int& exitCode) {
       std::array<char, 128> buffer;
       output.clear();
       FILE* pipe = popen((command + " 2>&1").c_str(), "r");
       if (!pipe) {
           std::cerr << "Error executing command" << std::endl;
           exitCode = -1;
           return false;
       }
       while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
           output += buffer.data();
       }
       exitCode = pclose(pipe);
       if (WIFSIGNALED(exitCode)) {
           int signal = WTERMSIG(exitCode);
           std::string signalName;
           switch (signal) {
               case SIGSEGV: signalName = "SIGSEGV (Segmentation fault)"; break;
               case SIGABRT: signalName = "SIGABRT (Aborted)"; break;
               case SIGBUS: signalName = "SIGBUS (Bus error)"; break;
               case SIGILL: signalName = "SIGILL (Illegal instruction)"; break;
               case SIGFPE: signalName = "SIGFPE (Floating point exception)"; break;
               default: signalName = "Signal " + std::to_string(signal); break;
           }
           output += "\nProcess terminated by signal: " + signalName;
           return false;
       }
       return exitCode == 0;
   }

   void logBug(const std::string& sourceFile, const std::string& compiler, const std::string& output) {
       std::ofstream logFile("bugs.log", std::ios::app);
       if (!logFile.is_open()) {
           std::cerr << "Failed to open bugs.log file" << std::endl;
           return;
       }
       logFile << "=== COMPILER CRASH DETECTED ===" << std::endl;
       logFile << "File: " << sourceFile << std::endl;
       logFile << "Compiler: " << compiler << std::endl;
       logFile << "Error output:" << std::endl;
       logFile << output << std::endl;
       logFile << "===============================" << std::endl << std::endl;
       logFile.close();
   }

public:
   void processSourceFile(const std::string& sourcePath) {
       if (!fs::exists(sourcePath)) {
           std::cerr << "Source file not found: " << sourcePath << std::endl;
           return;
       }
       std::string executableBase = "../test/" + fs::path(sourcePath).stem().string();
       
       fs::create_directories("../test");

       for (const auto& config : configs) {
           std::string executablePath = executableBase + "_" + config.name;
           std::string compileCommand = config.command + " " + sourcePath + " -o " + executablePath;

           std::string compileOutput;
           int exitCode;
           bool compileSuccess = executeCommand(compileCommand, compileOutput, exitCode);
           
           if (!compileSuccess && isCompilerCrash(compileOutput, exitCode)) {
               std::cout << "COMPILER CRASH DETECTED for " << config.name << " on file " << sourcePath << std::endl;
               std::cout << "Exit code: " << exitCode << std::endl;
               
               std::string enhancedOutput = "Exit code: " + std::to_string(exitCode) + "\n" + compileOutput;
               logBug(sourcePath, config.command, enhancedOutput);
           }
       }
   }

   void processAllFiles() {
       try {
           std::ofstream logFile("bugs.log", std::ios::trunc);
           logFile << "=== Compiler Crash Bug Log ===" << std::endl;
           logFile << "Date: " << __DATE__ << " " << __TIME__ << std::endl;
           logFile << "=============================\n\n";
           logFile.close();

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