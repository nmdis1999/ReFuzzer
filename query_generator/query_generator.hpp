#ifndef QUERY_GENERATOR_HPP
#define QUERY_GENERATOR_HPP

#include <iostream>
#include <string>
#include "include/ollama.hpp"

class QueryGenerator {
private:
  const std::string OLLAMA_MODEL = "llama3.2";

public:
  void loadModel() {
    try {
      ollama::pull(OLLAMA_MODEL);
      std::cout << "Model loaded successfully" << std::endl;
    } catch (const std::exception &e) {
      std::cerr << "Failed to load model: " << e.what() << std::endl;
    }
  }

  std::string askModel(const std::string &prompt) {
    try {
      ollama::options opts;
      opts.model = OLLAMA_MODEL;
      auto response = ollama::generate(prompt, opts);
      return response.response;
    } catch (const std::exception &e) {
      std::cerr << "Failed to get response: " << e.what() << std::endl;
      return "";
    }
  }
};

#endif // QUERY_GENERATOR_HPP
