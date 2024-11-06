#include "../query_generator//Parser.hpp"
#include "../query_generator/TestWriter.hpp"
#include "../query_generator/object_generator.hpp"
#include "../query_generator/query_generator.hpp"
#include <errno.h>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

std::vector<std::string> fileNames(const std::string &dirc) {
  std::vector<std::string> files;
  for (const auto &file : std::filesystem::directory_iterator(dirc)) {
    if (file.path().extension() == ".log") {
      std::string baseName = file.path().stem().string();
      files.push_back(baseName);
    }
  }
  return files;
}

int main(int argc, char *argv[]) {
  TestWriter writer;
  if (!writer.directoryExists("../log")) {
    std::cerr << "Log directory doesn't exist." << std::endl;
    return 1;
  }

  std::vector<std::string> files = fileNames("../log");
  for (const auto &file : files) {
    std::string logPath = "../log/" + file + ".log";
    std::string programPath = "../test/" + file + ".c";

    if (!std::filesystem::exists(programPath)) {
      std::cerr << "Program file not found: " << programPath << std::endl;
      continue;
    }

    std::string basePrompt =
        "Given the following C program and its compilation error log with -O0 "
        "optimization level, please analyze and correct the program to resolve "
        "the compilation errors.";

    QueryGenerator qGenerate;
    qGenerate.loadModel();

    std::string fullPrompt =
        qGenerate.generatePromptWithFiles(basePrompt, programPath, logPath);

    if (fullPrompt.empty()) {
      std::cerr << "Failed to generate prompt for " << file << std::endl;
      continue;
    } else {
      std::cout << "========================" << std::endl;
      std::cout << "Prompt: " << fullPrompt << std::endl;
    }

    std::string response = qGenerate.askModel(fullPrompt);
    if (response.empty()) {
      std::cerr << "NO response generated" << std::endl;
    }
    Parser parser;
    std::string regProgram = parser.getCProgram(response);
    std::cout << "============================" << std::endl;
    std::cout << "Program: \n" << regProgram << std::endl;
    std::cout << "============================" << std::endl;

    auto compile_cmd = parser.getGccCommand(response);
    auto [filepath, formatSuccess] = parser.parseAndSaveProgram(response, file);
    GenerateObject object;
    std::string objectPath = object.generateObjectFile(filepath, compile_cmd);
    if (objectPath.empty()) {
      std::cerr << "Error: failed generating object file for " << file
                << std::endl;
      continue;
    }
    return 0;
  }
  return 0;
}
