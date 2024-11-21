#ifndef PARSER_HPP
#define PARSER_HPP

#include "TestWriter.hpp"
#include <regex>
#include <string>
#include <utility>

class Parser {
public:
  struct ParseResult {
    std::string filepath;
    std::string gccCommand;
    std::string runtimeCommand;
    std::string cleanResponse;
    bool success;
  };

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
    try {
      std::regex codeBlockRegex("```c\\n([\\s\\S]*?)```");
      std::smatch match;

      if (std::regex_search(response, match, codeBlockRegex)) {
        return trim(match[1].str());
      }

      std::regex altCodeBlockRegex("```\\n([\\s\\S]*?)```");
      if (std::regex_search(response, match, altCodeBlockRegex)) {
        return trim(match[1].str());
      }
    } catch (const std::regex_error &e) {
      std::cerr << "Regex error: " << e.what() << "\n";
      size_t start = response.find("```c\n");
      if (start != std::string::npos) {
        start += 4;
        size_t end = response.find("\n```", start);
        if (end != std::string::npos) {
          return trim(response.substr(start, end - start));
        }
      }
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

  static std::pair<std::string, bool>
  parseAndSaveProgram(const std::string &response,
                      const std::string &filename = "") {
    std::string extractedProgram = getCProgram(response);
    if (extractedProgram.empty()) {
      std::cerr << "Failed to extract file" << std::endl;
      return {"", false};
    }

    TestWriter writer;
    std::string finalFilename = filename;
    std::string filepath = writer.writeFile(extractedProgram, finalFilename);
    if (filepath.empty()) {
      std::cerr << "Something went wrong while writing code to filename "
                << filepath << std::endl;
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
    result.cleanResponse = trim(response);

    return result;
  }
};

#endif // PARSER_HPP
