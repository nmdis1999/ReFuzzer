#include "../query_generator//Parser.hpp"
#include "../query_generator/TestWriter.hpp"
#include "../query_generator/object_generator.hpp"
#include "../query_generator/query_generator.hpp"
#include "../query_generator/sanitizer_processor.hpp"
#include <errno.h>
#include <filesystem>
#include <fstream>
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

bool checkForSanitizerErrors(const std::string& baseFilename) {
    std::string sanitizerLogPath = "../sanitizer_log/" + baseFilename + ".log";
    std::string crashLogPath = "../sanitizer_crash/" + baseFilename + ".log";

    return std::filesystem::exists(sanitizerLogPath) ||
           std::filesystem::exists(crashLogPath);
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

    QueryGenerator qGenerate("llama3.2");
    qGenerate.loadModel();

    std::string fullPrompt =
        qGenerate.generatePromptWithFiles(basePrompt, programPath, logPath);

    if (fullPrompt.empty()) {
      std::cerr << "Failed to generate prompt for " << file << std::endl;
      continue;
    }

    std::string response = qGenerate.askModel(fullPrompt);
    if (response.empty()) {
      std::cerr << "NO response generated" << std::endl;
      continue;
    }

    Parser parser;
    std::string regProgram = parser.getCProgram(response);
    auto [filepath, formatSuccess] = parser.parseAndSaveProgram(response, file);

    GenerateObject object;
    auto compile_cmd = parser.getGccCommand(response);
    std::string objectPath = object.generateObjectFile(filepath, compile_cmd);

    if (objectPath.empty()) {
      std::cerr << "Error: failed generating object file for " << file << std::endl;
      continue;
    }

    SanitizerProcessor sanitizer;
    sanitizer.processSourceFile(objectPath);

    bool hasSanitizerErrors = checkForSanitizerErrors(file);

    if (hasSanitizerErrors) {
      std::string runtimePrompt =
          "Given the following C program and its sanitizer error log with -O0 "
          "optimization level, please analyze and correct the program to resolve "
          "the sanitizer errors in the program.";

      // Get combined content from both log files if they exist
      std::string sanitizerContent;
      std::string sanitizerLogPath = "../sanitizer_log/" + file + ".log";
      std::string crashLogPath = "../sanitizer_crash/" + file + ".log";

      if (std::filesystem::exists(sanitizerLogPath)) {
          std::ifstream sanitizerLog(sanitizerLogPath);
          sanitizerContent += std::string((std::istreambuf_iterator<char>(sanitizerLog)),
                                        std::istreambuf_iterator<char>());
      }

      if (std::filesystem::exists(crashLogPath)) {
          std::ifstream crashLog(crashLogPath);
          sanitizerContent += std::string((std::istreambuf_iterator<char>(crashLog)),
                                        std::istreambuf_iterator<char>());
      }

      std::string sanitizerFullPrompt =
          qGenerate.generatePromptWithFiles(runtimePrompt, programPath, sanitizerContent);

      std::string sanitizerResponse = qGenerate.askModel(sanitizerFullPrompt);
      if (!sanitizerResponse.empty()) {
        auto [newFilepath, newFormatSuccess] =
            parser.parseAndSaveProgram(sanitizerResponse, file);

        std::string newObjectPath =
            object.generateObjectFile(newFilepath, compile_cmd);

        if (!newObjectPath.empty()) {
          sanitizer.processSourceFile(newObjectPath);

          bool stillHasErrors = checkForSanitizerErrors(file);
          if (!stillHasErrors) {
            if (std::filesystem::exists(sanitizerLogPath)) {
              std::filesystem::remove(sanitizerLogPath);
            }
            if (std::filesystem::exists(crashLogPath)) {
              std::filesystem::remove(crashLogPath);
            }
          }
        }
      }
    }
  }
  return 0;
}
