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

public:
  std::vector<std::string> getCommands(const std::string &response) {
    std::vector<std::string> commands;
    std::vector<std::string> markers = {"```bash",  "```BASH",  "```Bash",
                                        "```shell", "```SHELL", "```Shell",
                                        "```sh",    "```",      "```C"};

    std::string compileCmd = "", runtimeCmd = "";

    // First pass: check all markers for commands
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
        // Look for runtime command with regex
        std::regex runtimePattern(
            "\\.\\/[[:alnum:]_]+(?:[[:space:]]+[^`\\n]*)?");
        std::smatch matches;
        if (runtimeCmd.empty() &&
            std::regex_search(response, matches, runtimePattern)) {
          runtimeCmd = matches[0].str().substr(2); // Remove ./
        }

        // Look for compile command with regex
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
    std::string result;

    size_t start = response.find("```C");
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
