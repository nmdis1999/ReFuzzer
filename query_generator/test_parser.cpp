#include "Parser.hpp"
#include <cassert>
#include <iostream>
#include <string>

std::string normalize_whitespace(const std::string &str) {
  std::string result;
  bool last_was_space = true; // Start true to trim leading spaces

  for (char c : str) {
    if (std::isspace(c)) {
      if (!last_was_space) {
        result += ' ';
        last_was_space = true;
      }
    } else {
      result += c;
      last_was_space = false;
    }
  }

  if (!result.empty() && std::isspace(result.back())) {
    result.pop_back();
  }

  return result;
}

void test_basic_extraction() {
  Parser parser;
  std::string test_input = R"(
Here is some text before the code
```cpp
#include <iostream>

int main() {
    std::cout << "Hello World!" << std::endl;
    return 0;
}
This should not be included
```)";

  std::string expected = R"(#include <iostream>

int main() {
    std::cout << "Hello World!" << std::endl;
    return 0;
})";

  std::string result = parser.getCppProgram(test_input);

  std::string normalized_result = normalize_whitespace(result);
  std::string normalized_expected = normalize_whitespace(expected);

  if (normalized_result != normalized_expected) {
    std::cout << "Expected:\n[" << normalized_expected << "]\n";
    std::cout << "Got:\n[" << normalized_result << "]\n";
    assert(false && "Basic extraction test failed");
  }
}

void test_escaped_characters() {
  Parser parser;
  std::string test_input = R"(
```cpp
#include \u003ciostream\u003e

int main() {
    std::cout \u003c\u003c \"Hello World!\" \u003c\u003c std::endl;
    return 0;
}
```)";

  std::string expected = R"(#include <iostream>

int main() {
    std::cout << "Hello World!" << std::endl;
    return 0;
})";

  std::string result = parser.getCProgram(test_input);

  std::string normalized_result = normalize_whitespace(result);
  std::string normalized_expected = normalize_whitespace(expected);

  if (normalized_result != normalized_expected) {
    std::cout << "Expected:\n[" << normalized_expected << "]\n";
    std::cout << "Got:\n[" << normalized_result << "]\n";
    assert(false && "Escaped characters test failed");
  }
}

void test_no_code_block() {
  Parser parser;
  std::string test_input = "No code block here";
  std::string result = parser.getCProgram(test_input);
  assert(result.empty() && "No code block test failed");
}

int main() {
  std::cout << "Running parser tests..." << std::endl;

  try {
    test_basic_extraction();
    std::cout << "Basic extraction test passed" << std::endl;

    test_escaped_characters();
    std::cout << "Escaped characters test passed" << std::endl;

    test_no_code_block();
    std::cout << "No code block test passed" << std::endl;

    std::cout << "All tests passed!" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Test failed: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
