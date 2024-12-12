#include <errno.h>
#include <iostream>
#include <string>
// NOLINTNEXTLINE(build/include_subdir)
#include "Parser.hpp"
#include "PromptWriter.hpp"
#include "TestWriter.hpp"
#include "llm_tokens_options.hpp"
#include "object_generator.hpp"
#include "query_generator.hpp"
#include "sanitizer_processor.hpp"

int main(int argc, char *argv[]) {
  if (argc < 2 || argc > 2) {
    std::cout << "Please provide single parameter (0, 1, 2)\n";
    return 1;
  }

  int parameter;
  try {
    parameter = std::stoi(argv[1]);
  } catch (const std::invalid_argument &e) {
    std::cout
        << "Invalid parameter. Please provide a valid integer (0, 1, or 2)."
        << std::endl;
    return 1;
  }

  if (parameter < 0 || parameter > 2) {
    std::cout << "Invalid parameter. Please provide a value between 0 and 2 "
                 "(inclusive)."
              << std::endl;
    return 1;
  }

  if (parameter == 1) {
    LLMTokensOption llmIndexedTokens;

    std::string randomCompilerOpt = llmIndexedTokens.getRandomCompilerOpt();
    std::string randomCompilerParts = llmIndexedTokens.getRandomCompilerParts();
    std::string randomPL = llmIndexedTokens.getRandomPL();
    std::string compilerFlag = llmIndexedTokens.getRandomCompilerFlag();
    std::string optLevel = llmIndexedTokens.getRandomOptLevel();
    std::string prompt =
        "Coding task: give me a program in C "
        "with all includes. No input is "
        "taken."
        "Please return a program (C program) and a concrete example. "
        "The C program will be with code triggering with " +
        optLevel + " and program will cover " + randomCompilerOpt +
        " optimizations part of the compiler " + randomCompilerParts +
        ", and exercises this idea in C: " + randomPL +
        ". To recap the code contains these: " + optLevel + ", " +
        randomCompilerOpt + " and " + randomCompilerParts + " and " + randomPL +
        " make sure that the program has proper symbols instead of unicodes in "
        "your response."
        " and the program MUST be a C program. ";

    std::cout << "prompt" << prompt << std::endl;
    QueryGenerator qGenerate("llama2");
    qGenerate.loadModel();
    std::string response = qGenerate.askModel(prompt);
    // TODO: write constructor for the class Parser
    Parser parser;
    std::string program = parser.getCProgram(response);
    std::cout << "============================" << std::endl;
    std::cout << "Program: \n" << program << std::endl;
    std::cout << "============================" << std::endl;

    auto compile_cmd = parser.getGccCommand(response);
    auto runtime_cmd = parser.getRuntimeCommand(response);

    auto [filepath, formatSuccess] = parser.parseAndSaveProgram(response);
    if (filepath.empty()) {
      std::cerr << "Error: failed writing test file\n";
      return 1;
    }
    PromptWriter promptWriter;
    auto [promptPath, promptSuccess] =
        promptWriter.savePrompt(prompt, filepath, optLevel, randomCompilerOpt,
                                randomCompilerParts, randomPL);

    if (!promptSuccess) {
      std::cerr << "Error: failed saving prompt file\n";
      return 1;
    }
    GenerateObject object;
    std::string objectPath = object.generateObjectFile(filepath, compile_cmd);
    if (objectPath.empty()) {
      std::cerr << "Error: failed generating object file\n";
    }
    std::cout << "Running sanitizer checks on generated object files..." << std::endl;
  } else if (parameter == 2) {
    SanitizerProcessor sanitizer;
    for (const auto& entry : fs::directory_iterator("../object")) {
        if (entry.path().extension() == ".o") {
            sanitizer.processSourceFile(entry.path().string());
        }
    }
  } else {
    std::string res = argv[2];
    if (res.empty()) {
      std::cout << "Invalid parameter. Please provide a result text from "
                   "LLM model."
                << std::endl;
      return 1;
    }
  }

  return 0;
}
