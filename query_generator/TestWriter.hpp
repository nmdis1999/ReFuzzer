#ifndef TEST_WRITER
#define TEST_WRITER

#include <iostream>
#include <string>
#include <chrono>
#include <ctime>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <errno.h>
#include <iomanip>

class TestWriter {
private:
  bool directoryExists(const std::string &path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
      return false;
    }
    return (info.st_mode & S_IFDIR);
  }

  bool fileExists(const std::string &path) {
    struct stat info;
    return (stat(path.c_str(), &info) == 0);
  }

  bool createDirectory() {
    const std::string testDir = "../test";

    if (!directoryExists(testDir)) {
      if (mkdir(testDir.c_str(), 0755) != 0) {
        std::cerr << "Error creating directory: " << strerror(errno)
                  << std::endl;
        return false;
      }
      std::cout << "Test directory created successfully.\n";
    }
    return true;
  }

  std::string generateFilename(const std::string &prefix) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    std::stringstream ss;
    ss << prefix << "_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S")
       << "_" << ms.count() << ".c";

    return ss.str();
  }

public:
  std::string writeFile(const std::string &filename,
                        const std::string &content) {
    try {
      if (!createDirectory()) {
        return "";
      }
      std::string fullPath = "../test/" + filename;

      if (fileExists(fullPath)) {
        std::cerr << "File '" << fullPath << "' already exists" << std::endl;
        return "";
      }

      std::ofstream file(fullPath);
      if (!file.is_open()) {
        std::cerr << "Failed to open file '" << fullPath << "'" << std::endl;
        return "";
      }

      file << content;

      if (file.fail()) {
        std::cerr << "Error: Failed to write to file '" << fullPath << "'"
                  << std::endl;
        file.close();
        return "";
      }

      file.close();
      std::cout << "Code successfully written to '" << fullPath << "'"
                << std::endl;
      return fullPath;

    } catch (const std::exception &e) {
      std::cerr << "Error: " << e.what() << std::endl;
      return "";
    }
  }

  std::string writeFile(const std::string &content) {
    return writeFile(generateFilename("test_file_"), content);
  }
};

#endif
