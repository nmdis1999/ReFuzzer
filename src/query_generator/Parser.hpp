#ifndef PARSER_HPP
#define PARSER_HPP

#include "TestWriter.hpp"
#include <regex>
#include <string>
#include <utility>
#include <iostream>
#include <algorithm>
#include <cctype>

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
  
  static bool looksLikeCCode(const std::string& code) {
    bool hasInclude = code.find("#include") != std::string::npos;
    bool hasMain = code.find("int main") != std::string::npos;
    
    bool hasExplanationText = (code.find("This program demonstrates") != std::string::npos ||
                              code.find("The output of this program") != std::string::npos ||
                              code.find("demonstrates the use") != std::string::npos);
    
    return hasInclude && hasMain && !hasExplanationText;
  }

  static std::string cleanExtractedCode(const std::string& code) {
    std::string cleaned = code;
    
    size_t lastBrace = cleaned.rfind('}');
    if (lastBrace != std::string::npos) {
      std::string afterBrace = cleaned.substr(lastBrace + 1);
      afterBrace = trim(afterBrace);
      if (!afterBrace.empty() && 
          (afterBrace.find("This program") != std::string::npos ||
           afterBrace.find("The ") != std::string::npos ||
           afterBrace.find("demonstrates") != std::string::npos)) {
        cleaned = cleaned.substr(0, lastBrace + 1);
      }
    }
    
    return trim(cleaned);
  }

public:
  static std::string getCProgram(const std::string &response) {
    try {
      std::regex codeBlockRegex("```(?:c|cpp|c\\+\\+|C|CPP|C\\+\\+)\\s*\\n([\\s\\S]*?)```", std::regex_constants::icase);
      std::smatch match;

      if (std::regex_search(response, match, codeBlockRegex)) {
        std::cout << "Found code block with language specifier" << std::endl;
        std::string extracted = trim(match[1].str());
        return cleanExtractedCode(extracted);
      }

      std::regex altCodeBlockRegex("```\\s*\\n([\\s\\S]*?)```");
      if (std::regex_search(response, match, altCodeBlockRegex)) {
        std::string code = trim(match[1].str());
        std::cout << "Found generic code block" << std::endl;
        
        if (looksLikeCCode(code)) {
          return cleanExtractedCode(code);
        }
      }
      
      std::regex directCodeBlockRegex("```([cC]|[cC][pP][pP]|[cC]\\+\\+)?([\\s\\S]*?)```");
      if (std::regex_search(response, match, directCodeBlockRegex)) {
        std::string code = trim(match[2].str());
        std::cout << "Found direct code block" << std::endl;
        
        if (looksLikeCCode(code)) {
          return cleanExtractedCode(code);
        }
      }

    } catch (const std::regex_error &e) {
      std::cerr << "Regex error: " << e.what() << "\n";
    }
    
    std::cout << "Using manual parsing fallback" << std::endl;
    size_t start = response.find("```");
    while (start != std::string::npos) {
      size_t lang_end = response.find('\n', start);
      if (lang_end != std::string::npos) {
        std::string lang = response.substr(start + 3, lang_end - start - 3);
        lang = trim(lang);
        
        std::transform(lang.begin(), lang.end(), lang.begin(), ::tolower);
        if (lang == "c" || lang == "cpp" || lang == "c++" || lang.empty()) {
          size_t code_start = lang_end + 1;
          size_t code_end = response.find("```", code_start);
          if (code_end != std::string::npos) {
            std::string code = trim(response.substr(code_start, code_end - code_start));
            std::cout << "Manual parsing found code block" << std::endl;
            return cleanExtractedCode(code);
          }
        }
      }
      start = response.find("```", start + 3);
    }
    
    std::cout << "Trying pattern-based extraction" << std::endl;
    size_t includePos = response.find("#include");
    if (includePos != std::string::npos) {
      std::cout << "Found #include, extracting program" << std::endl;
      
      std::regex mainEndRegex("return\\s+0\\s*;\\s*}");
      std::smatch mainEndMatch;
      
      std::string searchArea = response.substr(includePos);
      if (std::regex_search(searchArea, mainEndMatch, mainEndRegex)) {
        size_t endPos = includePos + mainEndMatch.position() + mainEndMatch.length();
        std::string extracted = response.substr(includePos, endPos - includePos);
        return trim(extracted);
      }
      
      size_t lastBrace = response.rfind('}');
      if (lastBrace != std::string::npos && lastBrace > includePos) {
        std::string extracted = response.substr(includePos, lastBrace - includePos + 1);
        return cleanExtractedCode(extracted);
      }
    }
    
    std::cout << "Could not extract program" << std::endl;
    return "";
  }

  static std::pair<std::string, bool>
  parseAndSaveProgram(const std::string &response,
                      const std::string &filename = "", 
                      const std::string &dirName = "../test") {
    std::string extractedProgram = getCProgram(response);
    if (extractedProgram.empty()) {
      std::cerr << "Failed to extract file" << std::endl;
      return {"", false};
    }

    TestWriter writer;
    std::string finalFilename = filename;
    std::string filepath = writer.writeFile(extractedProgram, finalFilename, dirName);
    if (filepath.empty()) {
      std::cerr << "Something went wrong while writing code to filename "
                << finalFilename << std::endl;
      return {"", false};
    }

    return {filepath, true};
  }

  static ParseResult parse(const std::string &response, 
                          const std::string &filename = "", 
                          const std::string &dirName = "../test") {
    ParseResult result;

    auto [filepath, success] = parseAndSaveProgram(response, filename, dirName);
    result.filepath = filepath;
    result.success = success;
    result.cleanResponse = trim(response);
    return result;
  }
};

#endif // PARSER_HPP