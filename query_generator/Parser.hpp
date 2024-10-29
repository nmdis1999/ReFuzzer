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
        lineEnd = input.length();

      std::string line = input.substr(start, lineEnd - start);

      // Try to find gcc command first
      size_t gccStart = line.find("gcc");
      if (gccStart != std::string::npos) {
        std::string command = line.substr(gccStart);
        try {
          command = std::regex_replace(command, std::regex("`|'|\""), "");
          command =
              std::regex_replace(command, std::regex("^[ \\t]+|[ \\t]+$"), "");
        } catch (const std::regex_error &e) {
          command.erase(std::remove(command.begin(), command.end(), '`'),
                        command.end());
          command.erase(std::remove(command.begin(), command.end(), '\''),
                        command.end());
          command.erase(std::remove(command.begin(), command.end(), '\"'),
                        command.end());
          while (!command.empty() && std::isspace(command.front()))
            command.erase(0, 1);
          while (!command.empty() && std::isspace(command.back()))
            command.pop_back();
        }
        return "COMPILE:" + command;
      }

      // If no gcc command found, try to find runtime command (./)
      size_t runtimeStart = line.find("./");
      if (runtimeStart != std::string::npos) {
        std::string command = line.substr(runtimeStart + 2);
        try {
          command = std::regex_replace(command, std::regex("`|'|\""), "");
          command =
              std::regex_replace(command, std::regex("^[ \\t]+|[ \\t]+$"), "");
        } catch (const std::regex_error &e) {
          command.erase(std::remove(command.begin(), command.end(), '`'),
                        command.end());
          command.erase(std::remove(command.begin(), command.end(), '\''),
                        command.end());
          command.erase(std::remove(command.begin(), command.end(), '\"'),
                        command.end());
          while (!command.empty() && std::isspace(command.front()))
            command.erase(0, 1);
          while (!command.empty() && std::isspace(command.back()))
            command.pop_back();
        }
        return "RUNTIME:" + command;
      }

      return "0";
    } catch (const std::exception &e) {
      return "0";
    }
  }

  std::string processEscapeSequences(const std::string &input) {
    std::string processed;
    bool inQuotes = false;
    size_t i = 0;

    while (i < input.length()) {
      if (input[i] == '"' && (i == 0 || input[i - 1] != '\\')) {
        inQuotes = !inQuotes;
        processed += input[i];
        i++;
      } else if (!inQuotes && input[i] == '\\' && i + 1 < input.length()) {
        if (input[i + 1] == 'u' && i + 5 < input.length()) {
          // Handle \u#### format
          std::string hex = input.substr(i + 2, 4);
          int ascii = std::stoi(hex, nullptr, 16);
          processed += static_cast<char>(ascii);
          i += 6;
        } else {
          switch (input[i + 1]) {
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
            processed += input[i + 1];
          }
          i += 2;
        }
      } else {
        processed += input[i];
        i++;
      }
    }
    return processed;
  }

public:
  std::vector<std::string> getCommands(const std::string &response) {
    std::vector<std::string> commands;
    std::vector<std::string> markers = {"```bash",  "```BASH",  "```Bash",
                                        "```shell", "```SHELL", "```Shell",
                                        "```sh",    "```",      "```C"};

    std::string compileCmd = "", runtimeCmd = "";

    for (const auto &marker : markers) {
      std::string result = extractCommand(response, marker);
      if (result.substr(0, 8) == "COMPILE:" && compileCmd.empty()) {
        compileCmd = result.substr(8);
      } else if (result.substr(0, 8) == "RUNTIME:" && runtimeCmd.empty()) {
        runtimeCmd = result.substr(8);
      }
    }

    // Second pass: use regex if needed
    if (compileCmd.empty() || runtimeCmd.empty()) {
      try {
        std::regex runtimePattern(
            "\\.\\/[[:alnum:]_]+(?:[[:space:]]+[^`\\n]*)?");
        std::smatch matches;
        if (runtimeCmd.empty() &&
            std::regex_search(response, matches, runtimePattern)) {
          runtimeCmd = matches[0].str().substr(2); // Remove ./
        }

        std::regex compilePattern("gcc[^\\n`]*");
        if (compileCmd.empty() &&
            std::regex_search(response, matches, compilePattern)) {
          compileCmd = matches[0].str();
        }
      } catch (const std::regex_error &e) {
        // Fallback to basic string search if regex fails
        if (runtimeCmd.empty()) {
          size_t pos = response.find("./");
          if (pos != std::string::npos) {
            size_t endPos = response.find('\n', pos);
            if (endPos == std::string::npos)
              endPos = response.length();
            runtimeCmd = response.substr(pos + 2, endPos - (pos + 2));
          }
        }

        if (compileCmd.empty()) {
          size_t pos = response.find("gcc");
          if (pos != std::string::npos) {
            size_t endPos = response.find('\n', pos);
            if (endPos == std::string::npos)
              endPos = response.length();
            compileCmd = response.substr(pos, endPos - pos);
          }
        }
      }
    }

    if (!compileCmd.empty()) {
      commands.push_back(compileCmd);
    }
    if (!runtimeCmd.empty()) {
      commands.push_back(runtimeCmd);
    }

    return commands;
  }

  std::string getCProgram(const std::string &response) {
    std::vector<std::string> codeMarkers = {"```C", "```c", "```cpp", "```CPP"};

    for (const auto &startMarker : codeMarkers) {
      size_t start = response.find(startMarker);
      if (start != std::string::npos) {
        start = response.find('\n', start);
        if (start == std::string::npos)
          continue;
        start++;

        size_t end = response.find("```", start);
        if (end == std::string::npos)
          continue;

        std::string code = response.substr(start, end - start);

        code.erase(0, code.find_first_not_of(" \n\r\t"));
        code.erase(code.find_last_not_of(" \n\r\t") + 1);

        return processEscapeSequences(code);
      }
    }
    return "";
  }
};

#endif
