#ifndef LLM_TOKENS_OPTIONS_HPP
#define LLM_TOKENS_OPTIONS_HPP

#include <chrono>
#include <random>
#include <string>
#include <vector>

class LLMTokensOption {
private:
  std::vector<std::string> compilerOpt = {
      "loop unrolling", "function inlining", "constant folding",
      "dead code elimination", "register allocation"};

  // Compiler parts
  std::vector<std::string> compilerParts = {
      "front-end parsing", "intermediate representation",
      "back-end code generation", "optimization passes", "assembly generation"};

  // Programming language concepts
  std::vector<std::string> programmingLanguage = {
      "recursion", "array manipulation", "pointer arithmetic", "bit operations",
      "dynamic memory allocation"};

  std::mt19937 rng;

  template <typename T> T getRandomElement(const std::vector<T> &vec) {
    if (vec.empty())
      return T();

    std::uniform_int_distribution<size_t> dist(0, vec.size() - 1);
    return vec[dist(rng)];
  }

public:
  // Constructor Initiaze random number generator with current time
  LLMTokensOption()
      : rng(std::chrono::steady_clock::now().time_since_epoch().count()) {}

  std::string getRandomCompilerOpt() { return getRandomElement(compilerOpt); }
  std::string getRandomCompilerParts() {
    return getRandomElement(compilerParts);
  }
  std::string getRandomPL() { return getRandomElement(programmingLanguage); }
};
#endif
