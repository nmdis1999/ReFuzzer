#ifndef PROMPT_WRITER_H
#define PROMPT_WRITER_H

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

class PromptWriter {
private:
  std::string promptsDirectory;

  std::string sanitizeFilename(const std::string &filename) {
    std::string result = filename;
    const std::string invalid_chars = "\\/:*?\"<>|";
    for (char &c : result) {
      if (invalid_chars.find(c) != std::string::npos || !isprint(c)) {
        c = '_';
      }
    }
    return result;
  }

public:
  PromptWriter(const std::string &directory = "../prompt")
      : promptsDirectory(directory) {
    std::filesystem::create_directories(promptsDirectory);
  }

  std::pair<std::string, bool>
  savePrompt(const std::string &prompt, const std::string &sourceFilename,
             const std::string &optLevel, const std::string &compilerOpt,
             const std::string &compilerParts, const std::string &plFeature) {
    std::filesystem::path sourcePath(sourceFilename);
    std::string promptFilename = sourcePath.stem().string() + ".prompt";

    std::string fullPath =
        (std::filesystem::path(promptsDirectory) / promptFilename).string();

    try {
      std::ofstream promptFile(fullPath);
      if (!promptFile.is_open()) {
        return {std::string(), false};
      }

      promptFile << "Original Source: " << sourceFilename << "\n";
      promptFile << "Optimization Level: " << optLevel << "\n";
      promptFile << "Compiler Optimization: " << compilerOpt << "\n";
      promptFile << "Compiler Parts: " << compilerParts << "\n";
      promptFile << "Programming Language Feature: " << plFeature << "\n";
      promptFile << "\n=== PROMPT ===\n\n";
      promptFile << prompt << "\n";

      return {fullPath, true};
    } catch (const std::exception &e) {
      std::cerr << "Error saving prompt: " << e.what() << std::endl;
      return {std::string(), false};
    }
  }

  bool deletePrompt(const std::string &promptPath) {
    try {
      return std::filesystem::remove(promptPath);
    } catch (const std::exception &e) {
      std::cerr << "Error deleting prompt file: " << e.what() << std::endl;
      return false;
    }
  }
};

#endif // PROMPT_WRITER_H
