// parser.hpp
#ifndef PARSER_HPP
#define PARSER_HPP
#include <string>
class Parser {
public:
  std::string getCProgram(std::string response) {
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

    size_t pos;
    while ((pos = result.find("\\u003c")) != std::string::npos) {
      result.replace(pos, 6, "<");
    }
    while ((pos = result.find("\\u003e")) != std::string::npos) {
      result.replace(pos, 6, ">");
    }
    while ((pos = result.find("\\'")) != std::string::npos) {
      result.replace(pos, 2, "'");
    }
    while ((pos = result.find("\\\"")) != std::string::npos) {
      result.replace(pos, 2, "\"");
    }
    while ((pos = result.find("\\n")) != std::string::npos) {
      result.replace(pos, 2, "\n");
    }

    return result;
  }
};
#endif
