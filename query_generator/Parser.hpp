#ifndef PARSER_HPP
#define PARSER_HPP
#include <string>
#include <regex>

class Parser {
public:
  std::string getCppProgram(std::string response) {
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
