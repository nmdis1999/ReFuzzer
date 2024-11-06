#ifndef OBJECT_GENERATOR
#define OBJECT_GENERATOR

#include <array>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

class GenerateObject {
private:
  bool createDirectory(const std::string &dir) {
    struct stat info;
    if (stat(dir.c_str(), &info) != 0) {
      if (mkdir(dir.c_str(), 0755) != 0) {
        std::cerr << "Error creating directory " << dir << ": "
                  << strerror(errno) << std::endl;
        return false;
      }
      std::cout << "Directory created successfully: " << dir << std::endl;
    }
    return true;
  }

  std::string getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
  }

  void logError(const std::string &operation, const std::string &error,
                const std::string &sourceFile) {
    if (!createDirectory("../log")) {
      return;
    }

    size_t lastSlash = sourceFile.find_last_of("/\\");
    std::string filename = (lastSlash != std::string::npos)
                               ? sourceFile.substr(lastSlash + 1)
                               : sourceFile;

    size_t lastDot = filename.find_last_of('.');
    std::string baseName =
        (lastDot != std::string::npos) ? filename.substr(0, lastDot) : filename;

    std::string logFile = "../log/" + baseName + ".log";

    std::ofstream log(logFile, std::ios_base::app);
    if (log.is_open()) {
      log << "[" << getTimestamp() << "] Operation: " << operation << "\n";
      log << "Error: " << error << "\n";
      log << "----------------------------------------\n";
      log.close();
    }
  }

  bool createObjectDirectory() {
    if (!createDirectory("../object")) {
      logError("createObjectDirectory", "Failed to create object directory",
               "object_directory");
      return false;
    }
    return true;
  }

  std::string generateObjectFilename(const std::string &sourceFile) {
    size_t lastSlash = sourceFile.find_last_of("/\\");
    std::string filename = (lastSlash != std::string::npos)
                               ? sourceFile.substr(lastSlash + 1)
                               : sourceFile;

    size_t lastDot = filename.find_last_of('.');
    std::string baseName =
        (lastDot != std::string::npos) ? filename.substr(0, lastDot) : filename;
    return "../object/" + baseName + ".o";
  }

  bool executeCommand(const std::string &command, std::string &output,
                      const std::string &sourceFile) {
    std::array<char, 128> buffer;
    output.clear();

    std::string cmd = command + " 2>&1";
    FILE *pipe = popen(cmd.c_str(), "r");

    if (!pipe) {
      logError("executeCommand",
               "Failed to create pipe for command: " + command, sourceFile);
      return false;
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
      output += buffer.data();
    }

    int status = pclose(pipe);
    return status == 0;
  }

public:
  std::string generateObjectFile(const std::string &filename,
                                 const std::string gccCmd) {
    if (filename.empty()) {
      logError("generateObjectFile", "Source file path is empty",
               "empty_source");
      return "";
    }

    if (!createObjectDirectory()) {
      return "";
    }

    std::string objectFile = generateObjectFilename(filename);

    std::string finalGccCmd = "gcc " + filename + " -o " + objectFile;

    std::string commandOutput;
    if (!executeCommand(finalGccCmd, commandOutput, filename)) {
      if (!commandOutput.empty()) {
        logError("gcc compilation", commandOutput, filename);
      }
      std::cerr << "Compilation failed. Check log file for details."
                << std::endl;
      return "";
    }

    struct stat buffer;
    if (stat(objectFile.c_str(), &buffer) != 0) {
      logError("generateObjectFile",
               "Object file was not created: " + objectFile, filename);
      return "";
    }

    std::cout << "Successfully generated object file: " << objectFile
              << std::endl;
    return objectFile;
  }
};

#endif
