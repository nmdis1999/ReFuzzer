#ifndef SANITIZER_PROCESSOR_HPP
#define SANITIZER_PROCESSOR_HPP

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <array>
#include <sstream>
#include <cstdio>
namespace fs = std::filesystem;

class SanitizerProcessor {
private:
   bool createDirectory(const std::string& path) {
       try {
           if (!fs::exists(path)) {
               fs::create_directories(path);
               std::cout << "Created directory: " << path << std::endl;
           }
           return true;
       } catch (const fs::filesystem_error& e) {
           std::cerr << "Error creating directory " << path << ": " << e.what() << std::endl;
           return false;
       }
   }

   std::string getFileName(const std::string& objectPath) {
    fs::path path(objectPath);
    std::string filename = path.filename().string();
    return filename.substr(0, filename.find_last_of('.'));
   }

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

   void logError(const std::string& objectFile, const std::string& error) {
       if (!createDirectory("../sanitizer_log")) {
           std::cerr << "Failed to create log directory" << std::endl;
           return;
       }
       std::string logFile = "../sanitizer_log/" + getFileName(objectFile) + ".log";
       std::ofstream log(logFile);
       if (!log.is_open()) {
           std::cerr << "Failed to open log file: " << logFile << std::endl;
           return;
       }
       log << "Object File: " << objectFile << "\n";
       log << "Error Output:\n" << error << "\n";
       log.close();
       std::cout << "Error log written to: " << logFile << std::endl;
   }

public:
   void processSourceFile(const std::string& objectPath) {
   if (!createDirectory("../test")) return;

   std::string baseFilename = getFileName(objectPath);

   std::string sourcePath = "../test/" + baseFilename + ".c";

   if (!fs::exists(sourcePath)) {
       std::cerr << "Source file not found: " << sourcePath << std::endl;
       return;
   }

   std::string executablePath = "../test/" + baseFilename + "_sanitized";
   std::string command = "clang -fsanitize=address,undefined,leak,memory,thread "
                       "-fsanitize-address-use-after-scope "
                       "-fsanitize-memory-track-origins=2 "
                       "-fno-omit-frame-pointer "
                       "-fno-optimize-sibling-calls "
                       "-O1 -g " +
                       sourcePath + " -o " + executablePath;

   std::string output;
   bool success = executeCommand(command, output);

   if (!success) {
       std::cout << "Sanitizer check failed for: " << sourcePath << std::endl;
       std::cout << "Error output: " << output << std::endl;
       logError(sourcePath, output);
   } else {
       std::cout << "Sanitizer check passed for: " << sourcePath << std::endl;
   }
}

};

#endif
