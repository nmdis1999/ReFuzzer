#ifndef PARSER_HPP
#define PARSER_HPP

#include "CodeGen.hpp"
#include "TestWriter.hpp"
#include <regex>
#include <string>
#include <utility>

class Parser {
private:
  static std::string trim(const std::string &str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos)
      return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
  }

public:
  static std::string getCProgram(const std::string &response) {
    std::regex codeBlockRegex(R"(```c\n([\s\S]*?)```)");
    std::smatch match;
    if (std::regex_search(response, match, codeBlockRegex)) {
      std::string code = match[1].str();

      code = std::regex_replace(code, std::regex(R"(\\u003c)"), "<");
      code = std::regex_replace(code, std::regex(R"(\\u003e)"), ">");

      code = std::regex_replace(code, std::regex(R"(\\\n\s*)"), "");

      code = std::regex_replace(code, std::regex(R"(\\")"), "\"");
      code = std::regex_replace(code, std::regex(R"(\\n)"), "\n");

      return trim(code);
    }
    return "";
  }

  static std::string getGccCommand(const std::string &response) {
    std::regex gccRegex(R"(gcc[^\n]*\n)");
    std::smatch match;
    if (std::regex_search(response, match, gccRegex)) {
      return trim(match[0].str());
    }
    return "";
  }

  static std::string getRuntimeCommand(const std::string &response) {
    std::regex runtimeRegex(R"(\./[^\n]*\n)");
    std::smatch match;
    if (std::regex_search(response, match, runtimeRegex)) {
      return trim(match[0].str());
    }
    return "";
  }

public:
  struct ParseResult {
    std::string filepath;
    std::string gccCommand;
    std::string runtimeCommand;
    std::string cleanResponse;
    bool success;
  };

  static std::pair<std::string, bool>
  parseAndSaveProgram(const std::string &response) {
    std::string extractedProgram = getCProgram(response);
    if (extractedProgram.empty()) {
      return {"", false};
    }

    TestWriter writer;
    std::string filepath = writer.writeFile(extractedProgram);
    if (filepath.empty()) {
      return {"", false};
    }

    return {filepath, true};
  }

  static ParseResult parse(const std::string &response) {
    ParseResult result;

    auto [filepath, success] = parseAndSaveProgram(response);
    result.filepath = filepath;
    result.success = success;

    result.gccCommand = getGccCommand(response);
    result.runtimeCommand = getRuntimeCommand(response);

    std::string cleanResponse = response;
    cleanResponse = std::regex_replace(cleanResponse,
                                       std::regex(R"(```c\n[\s\S]*?```)"), "");
    cleanResponse =
        std::regex_replace(cleanResponse, std::regex(R"(gcc[^\n]*\n)"), "");
    cleanResponse =
        std::regex_replace(cleanResponse, std::regex(R"(\./[^\n]*\n)"), "");
    cleanResponse = std::regex_replace(
        cleanResponse, std::regex(R"(```(?:bash)?\n?```\n?)"), "");
    cleanResponse =
        std::regex_replace(cleanResponse, std::regex(R"(\n{3,})"), "\n\n");
    result.cleanResponse = trim(cleanResponse);

    return result;
  }
};

#endif // PARSER_HPP
