#include <errno.h>
#include <iostream>
#include <string>
// NOLINTNEXTLINE(build/include_subdir)
#include "Parser.hpp"
#include "TestWriter.hpp"
#include "llm_tokens_options.hpp"
#include "object_generator.hpp"
#include "query_generator.hpp"

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

  if (parameter == 0) {
    std::string promptStart =
        "Generate a concise C file without unnecessary includes, but with "
        "output. "
        "The C file should be a standard program, but with a twist: "
        "it replaces all constants in the main function with argument "
        "assignments, adds required includes.";
    promptStart += "Example ```C\n#include <stdio.h>\n#include <stdlib.h>\nint "
                   "main(int argc, char *argv[]) {\n"
                   "  if (argc != 2) {\n    printf(\"Usage: %s <value>\\n\", "
                   "argv[0]);\n    return 1;\n  }\n  int x = atoi(argv[1]);\n"
                   "  return x - 4;\n}```\nDo you think you can do that?";
    std::cout << promptStart << std::endl;
  } else if (parameter == 1) {
    LLMTokensOption llmIndexedTokens;

    std::string randomCompilerOpt = llmIndexedTokens.getRandomCompilerOpt();
    std::string randomCompilerParts = llmIndexedTokens.getRandomCompilerParts();
    std::string randomPL = llmIndexedTokens.getRandomPL();
    std::string compilerFlag = llmIndexedTokens.getRandomCompilerFlag();
    std::string optLevel = llmIndexedTokens.getRandomOptLevel();
    std::string prompt =
        "Coding task: give me a program in C "
        "with all includes. Input is "
        "taken via argv only. "
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
    QueryGenerator qGenerate;
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
    //
    GenerateObject object;
    std::string objectPath = object.generateObjectFile(filepath, compile_cmd);
    if (objectPath.empty()) {
      std::cerr << "Error: failed generating object file\n";
      return 1;
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
