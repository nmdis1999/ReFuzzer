#ifndef LLM_TOKENS_OPTIONS_HPP
#define LLM_TOKENS_OPTIONS_HPP

#include <iostream>
#include <chrono>
#include <random>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <stdexcept>
#include <cstdio>
#include <unordered_set>
#include <sstream>

class LLMTokensOption {
private:
  std::vector<std::string> compilerOpt = {
      "loop unrolling", "function inlining", "constant folding",
      "dead code elimination", "register allocation"};

  std::vector<std::string> compilerParts = {
      "front-end parsing", "intermediate representation",
      "back-end code generation", "optimization passes", "assembly generation"};

  std::vector<std::string> programmingLanguage = {
      "recursion", "array manipulation", "pointer arithmetic", "bit operations",
      "dynamic memory allocation"};

  std::vector<std::string> compilerFlags = {
      "-O0",   "-O1",           "-O2",      "-O3",        "-Os",
      "-Wall", "-Wextra",       "-Werror",  "-Wpedantic", "-Wunused",
      "-g",    "-ggdb",         "-g3",      "-fPIC",      "-m32",
      "-m64",  "-march=native", "-std=c99", "-std=c11"};

  std::vector<std::string> llvmPasses;
  std::mt19937 rng;

  std::vector<std::string> split_passes(const std::string &passes_string) {
    std::vector<std::string> parts;
    std::string temp_part;
    int nesting_level = 0;

    for (char c : passes_string) {
      if (c == '(' || c == '<') {
        nesting_level++;
      } else if (c == ')' || c == '>') {
        nesting_level--;
      }

      if (c == ',' && nesting_level == 0) {
        while (!temp_part.empty() && std::isspace(temp_part.back())) {
          temp_part.pop_back();
        }
        size_t start = temp_part.find_first_not_of(" \t\n\r");
        if (start != std::string::npos && !temp_part.empty()) {
          parts.push_back(temp_part.substr(start));
        }
        temp_part.clear();
      } else {
        temp_part += c;
      }
    }

    while (!temp_part.empty() && std::isspace(temp_part.back())) {
      temp_part.pop_back();
    }
    size_t start = temp_part.find_first_not_of(" \t\n\r");
    if (start != std::string::npos && !temp_part.empty()) {
      parts.push_back(temp_part.substr(start));
    }

    parts.erase(std::remove_if(parts.begin(), parts.end(),
                               [](const std::string &pass) {
                                 return pass == "BitcodeWriterPass";
                               }),
                parts.end());

    return parts;
  }

  std::string exec(const char *cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);

    if (!pipe) {
      throw std::runtime_error("popen() failed!");
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
      result += buffer.data();
    }
    return result;
  }

  /** Instead of manually filling tokens for middle-end
   * optimisation tokens we are now getting the list of
   * passes available for the middle-end pipeline via
   * llvm-opt command.
   * */
  void initializeLLVMPasses() {
    std::vector<std::string> opt_levels = {"O0", "O1", "O2", "O3"};
    std::vector<std::string> pass_types = {"default", "lto-pre-link", "lto"};

    for (const auto &opt : opt_levels) {
      for (const auto &type : pass_types) {
        try {
          std::string cmd = "opt -passes='" + type + "<" + opt +
                            ">' "
                            "-print-pipeline-passes /dev/null 2>/dev/null";
          std::string result = exec(cmd.c_str());
          auto passes = split_passes(result);
          llvmPasses.insert(llvmPasses.end(), passes.begin(), passes.end());
        } catch (const std::runtime_error &e) {
          std::cerr << "Error collecting passes for " << type << "<" << opt
                    << ">: " << e.what() << std::endl;
        }
      }
    }
  }

  template <typename T> T getRandomElement(const std::vector<T> &vec) {
    if (vec.empty())
      return T();

    std::uniform_int_distribution<size_t> dist(0, vec.size() - 1);
    return vec[dist(rng)];
  }

public:
  LLMTokensOption()
      : rng(std::chrono::steady_clock::now().time_since_epoch().count()) {
    std::cout << "Initializing LLVM passes..." << std::endl;

    // Check if opt exists
    FILE *pipe = popen("which opt", "r");
    if (!pipe) {
      std::cerr << "Error: Cannot check for opt command" << std::endl;
      return;
    }
    std::array<char, 128> buffer;
    std::string opt_path;
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
      opt_path += buffer.data();
    }
    pclose(pipe);

    if (opt_path.empty()) {
      std::cerr << "Warning: 'opt' command not found in PATH" << std::endl;
      return;
    }

    std::cout << "Found opt at: " << opt_path << std::endl;
    initializeLLVMPasses();
  }
  std::string getRandomCompilerOpt() { return getRandomElement(llvmPasses); }
  std::string getRandomCompilerParts() {
    return getRandomElement(compilerParts);
  }
  std::string getRandomCompilerFlag() {
    return getRandomElement(compilerFlags);
  }
  std::string getRandomPL() { return getRandomElement(programmingLanguage); }

  std::string getRandomLLVMPass() {
    for (auto pass : llvmPasses)
      std::cout << "pass: " << pass << std::endl;
    return getRandomElement(llvmPasses);
  }
};

#endif
