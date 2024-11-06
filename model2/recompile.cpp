#include "Parser.hpp"
#include "TestWriter.hpp"
#include "object_generator.hpp"
#include "query_generator.hpp"
#include <iostream>
#include <string>
#include <vector>

std::vector<std::string> fileName(const std::string &dirc) {
  std::vector<std::string> files;
  for (const auto &file : std::filesystem::directory_iterator(dirc)) {
    size_t lastSlash = file.find_last_of("/\\");
    std::string filename =
        (lastSlash != std::string::npos) ? file.substr(lastSlash + 1) : file;
    size_t lastDot = filename.find_last_of('.');
    std::string baseName =
        (lastDot != std::string::npos) ? filename.substr(0, lastDot) : filename;
    files.push_back(baseName);
  }
  return files;
}

int main(int argc, char *argv[]) {
  TestWriter writer;
  if (!writer.directoryExists("../log")) {
    std::cerr << "Log directory doesn't exists." << std::endl;
    return 1;
  }

  std::string prompt =
      "Given the C program" + program + " and the error log " + log +
      " received after compiling the code with -O0 optimisation level"
      " use the error log such that program doesn't throw those compilation "
      "errors."
      " To recap the use the program " +
      program + " and the error log " + log + " to correct the program. ";

  QueryGenerator qGenerate;
  qGenerate.loadModel();
  std::string response = qGenerate.askMode(prompt);
  Parser parser;
  std::string regProgram = parser.getCprogram(response);
  std::cout << "============================" << std::endl;
  std::cout << "Program: \n" << regProgram << std::endl;
  std::cout << "============================" << std::endl;

  auto compile_cmd = parser.getGccCommand(response);
  auto [filepath, formatSuccess] = parser.parseAndSaveProgram(response);

  GenerateObject object;
  std::string objectPath = object.generateObjectFile(filepath, compile_cmd);
  if (objectPath.empty()) {
    std::cerr << "Error: failed generating object file from model 2."
              << std::endl;
    return 1;
  }
  return 0;
}
