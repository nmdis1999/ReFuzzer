#include <iostream>
#include <string>
#include <errno.h>
// NOLINTNEXTLINE(build/include_subdir)
#include "llm_tokens_options.hpp"
#include "query_generator.hpp"
#include "Parser.hpp"
#include "TestWriter.hpp"

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
        "Generate a concise C++ file without unnecessary includes, but with "
        "output. "
        "The C file should be a standard program, but with a twist: "
        "it replaces all constants in the main function with argument "
        "assignments, adds required includes.";
    promptStart +=
        "Example ```C++\n#include <stdio.h>\n#include <stdlib.h>\nint "
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

    std::string prompt =
        "Coding task: give me a program in C++ with all includes. Input is "
        "taken "
        "via argv only. "
        "Please return a program (C++ program) and a concrete example of an "
        "The C++ program will be with code triggering " +
        randomCompilerOpt +
        " optimizations, covers this part of the compiler " +
        randomCompilerParts + ", and exercises this idea in C++: " + randomPL +
        ". To recap the code contains these: " + randomCompilerOpt + " and " +
        randomCompilerParts + " and " + randomPL;
    std::cout << "prompt" << prompt << std::endl;
    QueryGenerator qGenerate;
    qGenerate.loadModel();
    std::string response = qGenerate.askModel(prompt);
    std::cout << "response: " << response << std::endl;
    // TODO: write constructor for the class Parser

    Parser parser;
    std::string program = parser.getCppProgram(response);
    std::string command = parser.getArgsInput(response);
    std::cout << program << std::endl;
    std::cout << command << std::endl;
    // TODO: write constructor for the class TestWriter
    TestWriter writer;
    if (!writer.writeFile(program)) {
      std::cerr << "Error: failed writing test file\n";
    }
  } else {
    std::string res = argv[2];
    if (res.empty()) {
      std::cout
          << "Invalid parameter. Please provide a result text from LLM model."
          << std::endl;
      return 1;
    }

    // std::string callLine = parser.getArgsInput(res);
    // std::string argsType = parser.getTypes(res, callLine);
    //
    // std::cout << "argsType: " << argsType << std::endl;
    // std::cout << "callLine: " << callLine << std::endl;
    // std::cout << "program: " << program << std::endl;
  }

  return 0;
}
