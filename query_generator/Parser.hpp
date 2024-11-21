#ifndef PARSER_HPP
#define PARSER_HPP

#include "TestWriter.hpp"
#include <functional>
#include <regex>
#include <string>
#include <unordered_map>
#include <utility>

class Parser {
private:
  static const std::unordered_map<std::string, std::string> &getUnicodeMap() {
    static const std::unordered_map<std::string, std::string> unicodeMap = {
        {"\\u0020", " "},         {"\\u0021", "!"},
        {"\\u0022", "\""},        {"\\u0023", "#"},
        {"\\u0024", "$"},         {"\\u0025", "%"},
        {"\\u0026", "&"},         {"\\u0027", "'"},
        {"\\u0028", "("},         {"\\u0029", ")"},
        {"\\u002A", "*"},         {"\\u002B", "+"},
        {"\\u002C", ","},         {"\\u002D", "-"},
        {"\\u002E", "."},         {"\\u002F", "/"},
        {"\\u003A", ":"},         {"\\u003B", ";"},
        {"\\u003C", "<"},         {"\\u003D", "="},
        {"\\u003E", ">"},         {"\\u003F", "?"},
        {"\\u0040", "@"},         {"\\u005B", "["},
        {"\\u005C", "\\"},        {"\\u005D", "]"},
        {"\\u005E", "^"},         {"\\u005F", "_"},
        {"\\u0060", "`"},         {"\\u007B", "{"},
        {"\\u007C", "|"},         {"\\u007D", "}"},
        {"\\u007E", "~"},         {"u003c", "<"},
        {"u003e", ">"},           {"\\u003c", "<"},
        {"\\u003e", ">"},         {"&", "&"},
        {"&amp;", "&"},           {"u0026u0026", "&&"},
        {"\\u0026\\u0026", "&&"},
    };
    return unicodeMap;
  }

  static std::string convertUnicodeSequences(const std::string &input) {
    std::string result = input;
    const auto &unicodeMap = getUnicodeMap();

    for (const auto &[unicode, replacement] : unicodeMap) {
      size_t pos = 0;
      while ((pos = result.find(unicode, pos)) != std::string::npos) {
        result.replace(pos, unicode.length(), replacement);
        pos += replacement.length();
      }
    }

    std::regex unicodeRegex(R"(\\u([0-9a-fA-F]{4}))");
    std::string processed;
    std::string::const_iterator searchStart(result.cbegin());
    std::smatch matches;

    while (
        std::regex_search(searchStart, result.cend(), matches, unicodeRegex)) {
      processed += std::string(searchStart, matches[0].first);
      int codePoint = std::stoi(matches[1].str(), nullptr, 16);
      if (codePoint < 128) {
        processed += static_cast<char>(codePoint);
      } else {
        processed += matches[0].str();
      }
      searchStart = matches[0].second;
    }
    processed += std::string(searchStart, result.cend());

    processed = std::regex_replace(processed, std::regex("&amp;"), "&");
    processed = std::regex_replace(processed, std::regex("\\\\n"), "\n");

    return processed;
  }

  static std::string trim(const std::string &str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos)
      return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
  }

public:
  static std::string getCProgram(const std::string &raw_response) {
    std::regex jsonResponseRegex(
        "\"response\":\\s*\"([^\"\\\\]*(?:\\\\.[^\"\\\\]*)*)\"");
    std::smatch jsonMatch;
    std::string response;

    try {
      if (std::regex_search(raw_response, jsonMatch, jsonResponseRegex)) {
        response = jsonMatch[1].str();
        response = std::regex_replace(response, std::regex("\\\\n"), "\n");
        response = std::regex_replace(response, std::regex("\\\\\""), "\"");
      } else {
        response = raw_response;
      }

      std::regex codeBlockRegex("```c\\n([\\s\\S]*?)```");
      std::smatch match;

      if (std::regex_search(response, match, codeBlockRegex)) {
        std::string code = match[1].str();
        code = convertUnicodeSequences(code);
        return trim(code);
      }

      std::regex altCodeBlockRegex("```\\n([\\s\\S]*?)```");
      if (std::regex_search(response, match, altCodeBlockRegex)) {
        std::string code = match[1].str();
        code = convertUnicodeSequences(code);
        return trim(code);
      }
    } catch (const std::regex_error &e) {
      std::cerr << "Regex error: " << e.what() << "\n";
      size_t start = response.find("```c\n");
      if (start != std::string::npos) {
        start += 4;
        size_t end = response.find("\n```", start);
        if (end != std::string::npos) {
          std::string code = response.substr(start, end - start);
          code = convertUnicodeSequences(code);
          return trim(code);
        }
      }
    }

    return "";
  }
  static std::string getGccCommand(const std::string &response) {
    std::regex gccRegex(R"(gcc[^\n]*\n)");
    std::smatch match;
    if (std::regex_search(response, match, gccRegex)) {
      return trim(convertUnicodeSequences(match[0].str()));
    }
    return "";
  }

  static std::string getRuntimeCommand(const std::string &response) {
    std::regex runtimeRegex(R"(\./[^\n]*\n)");
    std::smatch match;
    if (std::regex_search(response, match, runtimeRegex)) {
      return trim(convertUnicodeSequences(match[0].str()));
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

    std::string cleanResponse = convertUnicodeSequences(response);
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
