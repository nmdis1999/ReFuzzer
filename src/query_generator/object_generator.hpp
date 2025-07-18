#ifndef OBJECT_GENERATOR
#define OBJECT_GENERATOR

#include <array>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <cstring>
#include <sys/stat.h>

class GenerateObject {
private:
  std::string objectFile;
  std::string workingDir;

  bool createDirectory(const std::string &dir) {
    try {
      std::filesystem::create_directories(dir);
      return true;
    } catch (const std::filesystem::filesystem_error& e) {
      std::cerr << "Error creating directory " << dir << ": " << e.what() << std::endl;
      return false;
    }
  }

  std::string getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
  }

  std::string ensureTrailingSlash(const std::string &path) {
    if (path.empty()) return "./";
    if (path.back() != '/') {
      return path + "/";
    }
    return path;
  }

  void logError(const std::string &operation, const std::string &error,
                const std::string &sourceFile, const std::string &dirName = "../test") {
    
    std::string logDir = ensureTrailingSlash(dirName) + "log";
    
    if (!createDirectory(logDir)) {
      std::cerr << "Failed to create log directory: " << logDir << std::endl;
      return;
    }
    size_t lastSlash = sourceFile.find_last_of("/\\");
    std::string filename = (lastSlash != std::string::npos)
                               ? sourceFile.substr(lastSlash + 1)
                               : sourceFile;
    size_t lastDot = filename.find_last_of('.');
    std::string baseName =
        (lastDot != std::string::npos) ? filename.substr(0, lastDot) : filename;

    std::string logFile = logDir + "/" + baseName + ".log";

    std::ofstream log(logFile, std::ios_base::out | std::ios_base::app);
    if (log.is_open()) {
      log << "[" << getTimestamp() << "] Operation: " << operation << "\n";
      log << "Source File: " << sourceFile << "\n";
      log << "Error: " << error << "\n";
      log << "----------------------------------------\n";
      log.close();
      std::cout << "Error logged to: " << logFile << std::endl;
    } else {
      std::cerr << "Failed to open log file: " << logFile << std::endl;
    }
  }

  bool createObjectDirectory(const std::string &dirName) {
    std::string objectDir = ensureTrailingSlash(dirName) + "object";
    
    if (!createDirectory(objectDir)) {
      logError("createObjectDirectory", "Failed to create object directory: " + objectDir,
               "object_directory", dirName);
      return false;
    }
    return true;
  }

  std::string generateObjectFilename(const std::string &sourceFile, const std::string &dirName) {
    size_t lastSlash = sourceFile.find_last_of("/\\");
    std::string filename = (lastSlash != std::string::npos)
                               ? sourceFile.substr(lastSlash + 1)
                               : sourceFile;

    size_t lastDot = filename.find_last_of('.');
    std::string baseName =
        (lastDot != std::string::npos) ? filename.substr(0, lastDot) : filename;
    
    std::string objectDir = ensureTrailingSlash(dirName) + "object";
    return objectDir + "/" + baseName + ".o";
  }

  std::string getLogFilePath(const std::string &sourceFile, const std::string &dirName) {
    size_t lastSlash = sourceFile.find_last_of("/\\");
    std::string filename = (lastSlash != std::string::npos)
                               ? sourceFile.substr(lastSlash + 1)
                               : sourceFile;

    size_t lastDot = filename.find_last_of('.');
    std::string baseName =
        (lastDot != std::string::npos) ? filename.substr(0, lastDot) : filename;
    
    std::string logDir = ensureTrailingSlash(dirName) + "log";
    return logDir + "/" + baseName + ".log";
  }

  bool executeCommand(const std::string &command, std::string &output,
                      const std::string &sourceFile, const std::string &dirName) {
    std::array<char, 128> buffer;
    output.clear();
    std::string cmd = command + " 2>&1";
    
    std::cout << "Executing: " << command << std::endl;
    
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
      logError("executeCommand",
               "Failed to create pipe for command: " + command, sourceFile, dirName);
      return false;
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
      output += buffer.data();
    }

    int status = pclose(pipe);
    return status == 0;
  }

public:
  std::string getObjectFileName() const {
    return std::filesystem::path(objectFile).filename().string();
  }

  std::string getWorkingDirectory() const {
    return workingDir;
  }

  std::string generateObjectFile(const std::string &filename, const std::string &dirName = "../test") {
    if (filename.empty()) {
      logError("generateObjectFile", "Source file path is empty",
               "empty_source", dirName);
      return "";
    }

    if (!std::filesystem::exists(filename)) {
      logError("generateObjectFile", "Source file does not exist: " + filename,
               filename, dirName);
      return "";
    }

    workingDir = dirName;

    if (!createObjectDirectory(dirName)) {
      return "";
    }

    std::string objectFilePath = generateObjectFilename(filename, dirName);
    this->objectFile = objectFilePath;
    
    std::string logFile = getLogFilePath(filename, dirName);

    std::string finalClangCmd = "clang++ " + filename + " -o " + objectFilePath;

    std::string commandOutput;
    if (!executeCommand(finalClangCmd, commandOutput, filename, dirName)) {
      if (!commandOutput.empty()) {
        logError("clang compilation", commandOutput, filename, dirName);
      } else {
        logError("clang compilation", "Compilation failed with unknown error", filename, dirName);
      }
      std::cerr << "Compilation failed for: " << filename << std::endl;
      std::cerr << "Check log file for details: " << logFile << std::endl;
      return "";
    }

    if (!std::filesystem::exists(objectFilePath)) {
      logError("generateObjectFile",
               "Object file was not created: " + objectFilePath, filename, dirName);
      return "";
    }

    if (std::filesystem::exists(logFile)) {
      try {
        std::filesystem::remove(logFile);
        std::cout << "Removed old log file: " << logFile << std::endl;
      } catch (const std::filesystem::filesystem_error &e) {
        std::cerr << "Warning: Failed to remove old log file: " << e.what() << std::endl;
      }
    }

    std::cout << "Successfully generated object file: " << objectFilePath << std::endl;
    return objectFilePath;
  }

  bool removeObjectFile(const std::string &objectPath = "") {
    std::string targetFile = objectPath.empty() ? this->objectFile : objectPath;
    
    if (targetFile.empty()) {
      std::cerr << "No object file to remove" << std::endl;
      return false;
    }

    try {
      if (std::filesystem::exists(targetFile)) {
        std::filesystem::remove(targetFile);
        std::cout << "Removed object file: " << targetFile << std::endl;
        return true;
      } else {
        std::cout << "Object file does not exist: " << targetFile << std::endl;
        return false;
      }
    } catch (const std::filesystem::filesystem_error& e) {
      std::cerr << "Error removing object file: " << e.what() << std::endl;
      return false;
    }
  }
  std::vector<std::string> getObjectFiles(const std::string &dirName = "../test") {
    std::vector<std::string> objectFiles;
    std::string objectDir = ensureTrailingSlash(dirName) + "object";
    
    try {
      if (std::filesystem::exists(objectDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(objectDir)) {
          if (entry.path().extension() == ".o") {
            objectFiles.push_back(entry.path().string());
          }
        }
      }
    } catch (const std::filesystem::filesystem_error& e) {
      std::cerr << "Error reading object directory: " << e.what() << std::endl;
    }
    
    return objectFiles;
  }
};

#endif