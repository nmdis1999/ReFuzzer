#ifndef OBJECT_GENERATOR
#define OBJECT_GENERATOR

#include "TestWriter.hpp"
#include <cstdlib>
#include <sys/stat.h>

class generateObject {
private:
  bool createObjectDirectory() {
    const std::string objDir = "../object";

    struct stat info;
    if (stat(objDir.c_str(), &info) != 0) {
      if (mkdir(objDir.c_str(), 0755) != 0) {
        std::cerr << "Error creating object directory: " << strerror(errno)
                  << std::endl;
        return false;
      }
      std::cout << "Object directory created successfully.\n";
    }
    return true;
  }

  std::string generateObjectFilename(const std::string &sourceFile) {
    size_t lastSlash = sourceFile.find_last_of("/");
    std::string baseName = (lastSlash == std::string::npos)
                               ? sourceFile
                               : sourceFile.substr(lastSlash + 1);

    size_t lastDot = baseName.find_last_of('.');
    if (lastDot != std::string::npos) {
      baseName = baseName.substr(0, lastDot);
    }

    return "../object/" + baseName + ".o";
  }

public:
  std::string generateObjectFile(const std::string &filename,
                                 const std::vector<std::string> &commands) {
    if (commands.empty()) {
      std::cerr << "No commands provided" << std::endl;
      return "";
    }

    if (!createObjectDirectory()) {
      std::cerr << "Failed to create object directory" << std::endl;
      return "";
    }

    std::string sourceFile = filename;

    if (sourceFile.empty()) {
      std::cerr << "Source file path is empty" << std::endl;
      return "";
    }

    std::string objectFile = generateObjectFilename(sourceFile);

    std::string gccCmd = "gcc -c " + sourceFile + " -o " + objectFile;

    int result = std::system(gccCmd.c_str());
    if (result == 0) {
      std::cout << "Successfully generated object file: " << objectFile
                << std::endl;
      return objectFile;
    } else {
      std::cerr << "Failed to generate object file. Command: " << gccCmd
                << std::endl;
      return "";
    }
  }
};

#endif
