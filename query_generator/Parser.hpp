#ifndef PARSER_HPP
#define PARSER_HPP

#include <algorithm>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

class Parser {
private:
  std::string extractCommand(const std::string &input,
                             const std::string &marker) {
    try {
      size_t start = input.find(marker);
      if (start == std::string::npos)
        return "";

      size_t lineEnd = input.find('\n', start);
      if (lineEnd == std::string::npos)
        return "";

      std::string line = input.substr(start, lineEnd - start);

      size_t cmdStart = line.find("./");
      if (cmdStart == std::string::npos)
        return "0";

      std::string command = line.substr(cmdStart + 2);

      // Simplified regex patterns with proper escaping
      try {
        // Remove backticks and quotes
        command = std::regex_replace(command, std::regex("`|'|\""), "");
        // Trim leading/trailing whitespace
        command =
            std::regex_replace(command, std::regex("^[ \\t]+|[ \\t]+$"), "");
      } catch (const std::regex_error &e) {
        // If regex fails, fall back to manual cleaning
        command.erase(std::remove(command.begin(), command.end(), '`'),
                      command.end());
        command.erase(std::remove(command.begin(), command.end(), '\''),
                      command.end());
        command.erase(std::remove(command.begin(), command.end(), '\"'),
                      command.end());
        // Trim whitespace manually
        while (!command.empty() && std::isspace(command.front()))
          command.erase(0, 1);
        while (!command.empty() && std::isspace(command.back()))
          command.pop_back();
      }

      return command;
    } catch (const std::exception &e) {
      return "0";
    }
  }

public:
  std::string getArgsInput(const std::string &response) {
    std::vector<std::string> markers = {"```bash",  "```BASH",  "```Bash",
                                        "```shell", "```SHELL", "```Shell",
                                        "```sh",    "```"};

    for (const auto &marker : markers) {
      std::string result = extractCommand(response, marker);
      if (!result.empty() && result != "0") {
        return result;
      }
    }

    try {
      // Fixed regex pattern
      std::regex cmdPattern("\\.\\/[[:alnum:]_]+(?:[[:space:]]+[^`\\n]*)?");
      std::smatch matches;
      if (std::regex_search(response, matches, cmdPattern)) {
        std::string command = matches[0];
        // Remove ./ from the start
        command = command.substr(2);
        return command;
      }
    } catch (const std::regex_error &e) {
      // Fallback: basic string search if regex fails
      size_t pos = response.find("./");
      if (pos != std::string::npos) {
        size_t endPos = response.find('\n', pos);
        if (endPos == std::string::npos)
          endPos = response.length();
        std::string command = response.substr(pos + 2, endPos - (pos + 2));
        // Basic cleanup
        command.erase(std::remove(command.begin(), command.end(), '`'),
                      command.end());
        while (!command.empty() && std::isspace(command.front()))
          command.erase(0, 1);
        while (!command.empty() && std::isspace(command.back()))
          command.pop_back();
        return command;
      }
    }

    return "0";
  }

  std::string getCppProgram(const std::string &response) {
    std::string result;

    size_t start = response.find("```cpp");
    if (start == std::string::npos) {
      return "";
    }
    start += 6;
    size_t end = response.find_last_of('}', response.find("```", start));
    if (end == std::string::npos) {
      return "";
    }
    end += 1;

    result = response.substr(start, end - start);

    std::string processed;
    bool inQuotes = false;
    size_t i = 0;

    while (i < result.length()) {
      if (result[i] == '"' && (i == 0 || result[i - 1] != '\\')) {
        inQuotes = !inQuotes;
        processed += result[i];
        i++;
      } else if (!inQuotes && result[i] == '\\' && i + 1 < result.length()) {
        if (result[i + 1] == 'u' && i + 5 < result.length()) {
          // Handle \u#### format
          std::string hex = result.substr(i + 2, 4);
          int ascii = std::stoi(hex, nullptr, 16);
          processed += static_cast<char>(ascii);
          i += 6;
        } else {
          switch (result[i + 1]) {
          case 'n':
            processed += '\n';
            break;
          case 't':
            processed += '\t';
            break;
          case 'r':
            processed += '\r';
            break;
          case '0':
            processed += '\0';
            break;
          case '\\':
            processed += '\\';
            break;
          default:
            processed += result[i + 1];
          }
          i += 2;
        }
      } else {
        processed += result[i];
        i++;
      }
    }

    return processed;
  }
};

#endif
