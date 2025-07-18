#ifndef TEST_WRITER
#define TEST_WRITER

#include <chrono>
#include <cstring>
#include <ctime>
#include <errno.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>

class TestWriter {
public:
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

  bool createDirectory(const std::string &dirName) {
    std::string testDir = dirName.empty() ? "../test" : dirName;
    
    if (!directoryExists(testDir)) {
      if (mkdir(testDir.c_str(), 0755) != 0) {
        std::cerr << "Error creating directory '" << testDir << "': " 
                  << strerror(errno) << std::endl;
        return false;
      }
      std::cout << "Directory '" << testDir << "' created successfully.\n";
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
       << "_" << ms.count() << ".cpp";

    return ss.str();
  }

  std::string ensureTrailingSlash(const std::string &path) {
    if (path.empty()) return "./";
    if (path.back() != '/') {
      return path + "/";
    }
    return path;
  }

public:
  std::string writeFile(const std::string &filename,
                        const std::string &content, 
                        const std::string &dirName = "../test") {
    try {
      if (!createDirectory(dirName)) {
        return "";
      }
      
      std::string fullPath = ensureTrailingSlash(dirName) + filename;

      std::ofstream file(fullPath, std::ios::out | std::ios::trunc);
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

  std::string writeFile(const std::string &content, 
                        std::string &filename,
                        const std::string &dirName = "../test") {
    if (filename.empty()) {
      filename = generateFilename("test_file");
    } else if (filename.find(".cpp") == std::string::npos) {
      filename += ".c";
    }
    
    std::cout << "Generated filename: " << filename << std::endl;
    return writeFile(filename, content, dirName);
  }
};

#endif