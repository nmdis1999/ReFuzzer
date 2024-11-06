#ifndef QUERY_GENERATOR_HPP
#define QUERY_GENERATOR_HPP

#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

class QueryGenerator {
private:
  const std::string OLLAMA_MODEL = "llama3.2";
  const std::string base_url;
  CURL *curl;

  static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                              void *userp) {
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
  }

  std::string escapeJsonString(const std::string &input) {
    std::string output;
    output.reserve(input.length());

    for (char ch : input) {
      switch (ch) {
      case '"':
        output += "\\\"";
        break;
      case '\\':
        output += "\\\\";
        break;
      case '\b':
        output += "\\b";
        break;
      case '\f':
        output += "\\f";
        break;
      case '\n':
        output += "\\n";
        break;
      case '\r':
        output += "\\r";
        break;
      case '\t':
        output += "\\t";
        break;
      default:
        if (static_cast<unsigned char>(ch) < 0x20) {
          char buf[8];
          snprintf(buf, sizeof(buf), "\\u%04x", ch);
          output += buf;
        } else {
          output += ch;
        }
      }
    }
    return output;
  }

  std::string extract_response(const std::string &json_response) {
    std::cout << "**********Raw JSON response: ********" << std::endl;
    std::cout << json_response << std::endl;
    std::cout << "************************************" << std::endl;

    size_t response_start = json_response.find("\"response\":\"");
    if (response_start == std::string::npos) {
      return "Could not find response field in JSON";
    }

    response_start += 11; // Length of "response":"
    size_t response_end = json_response.find("\",\"done\":", response_start);

    if (response_end == std::string::npos) {
      return "Malformed JSON response";
    }

    std::string extracted_response =
        json_response.substr(response_start, response_end - response_start);

    // Unescape the response
    std::string unescaped;
    size_t pos = 0;
    while (pos < extracted_response.length()) {
      if (extracted_response[pos] == '\\') {
        if (pos + 1 < extracted_response.length()) {
          switch (extracted_response[pos + 1]) {
          case 'n':
            unescaped += '\n';
            break;
          case 'r':
            unescaped += '\r';
            break;
          case 't':
            unescaped += '\t';
            break;
          case '\"':
            unescaped += '\"';
            break;
          case '\\':
            unescaped += '\\';
            break;
          default:
            unescaped += extracted_response[pos + 1];
          }
          pos += 2;
        } else {
          unescaped += extracted_response[pos++];
        }
      } else {
        unescaped += extracted_response[pos++];
      }
    }
    return unescaped;
  }

public:
  QueryGenerator(const std::string &host = "localhost", int port = 11434)
      : base_url("http://" + host + ":" + std::to_string(port)) {
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (!curl) {
      throw std::runtime_error("Failed to initialize CURL");
    }
  }

  ~QueryGenerator() {
    if (curl) {
      curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
  }

  void loadModel() {
    try {
      std::string json_body = "{\"model\":\"" + OLLAMA_MODEL + "\"}";
      std::string response_string;

      std::string url = base_url + "/api/pull";
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body.c_str());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json_body.length());

      struct curl_slist *headers = NULL;
      headers = curl_slist_append(headers, "Content-Type: application/json");
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

      CURLcode res = curl_easy_perform(curl);
      curl_slist_free_all(headers);

      if (res != CURLE_OK) {
        throw std::runtime_error(std::string("Failed to pull model: ") +
                                 curl_easy_strerror(res));
      }

      std::cout << "Model loaded successfully" << std::endl;
    } catch (const std::exception &e) {
      std::cerr << "Failed to load model: " << e.what() << std::endl;
    }
  }

  std::string generatePromptWithFiles(const std::string &basePrompt,
                                      const std::string &programPath,
                                      const std::string &logPath = "") {
    try {
      if (!std::filesystem::exists(programPath)) {
        throw std::runtime_error("Program file does not exist: " + programPath);
      }

      std::ifstream progFile(programPath);
      if (!progFile.is_open()) {
        throw std::runtime_error("Could not open program file: " + programPath);
      }
      std::stringstream progBuffer;
      progBuffer << progFile.rdbuf();
      std::string programContent = progBuffer.str();

      std::string fullPrompt = basePrompt;
      fullPrompt += "\nProgram Content:\n```c\n" + programContent + "\n```\n";

      if (!logPath.empty()) {
        if (!std::filesystem::exists(logPath)) {
          throw std::runtime_error("Log file does not exist: " + logPath);
        }
        std::ifstream logFile(logPath);
        if (!logFile.is_open()) {
          throw std::runtime_error("Could not open log file: " + logPath);
        }
        std::stringstream logBuffer;
        logBuffer << logFile.rdbuf();
        std::string logContent = logBuffer.str();
        fullPrompt += "\nCompilation Log:\n```\n" + logContent + "\n```\n";
      }

      return fullPrompt;
    } catch (const std::exception &e) {
      std::cerr << "Error generating prompt: " << e.what() << std::endl;
      return "";
    }
  }

  std::string askModel(const std::string &prompt) {
    try {
      if (!curl) {
        throw std::runtime_error("CURL not initialized");
      }

      std::string escaped_prompt = escapeJsonString(prompt);
      std::string json_body = "{\"model\":\"" + OLLAMA_MODEL +
                              "\",\"prompt\":\"" + escaped_prompt +
                              "\",\"stream\":false}";

      // Debug: Print the JSON body
      std::cout << "Sending JSON body:" << std::endl;
      std::cout << json_body << std::endl;

      std::string response_string;
      std::string url = base_url + "/api/generate";
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body.c_str());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json_body.length());

      struct curl_slist *headers = NULL;
      headers = curl_slist_append(headers, "Content-Type: application/json");
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

      CURLcode res = curl_easy_perform(curl);
      curl_slist_free_all(headers);

      if (res != CURLE_OK) {
        throw std::runtime_error(std::string("Failed to get response: ") +
                                 curl_easy_strerror(res));
      }

      return extract_response(response_string);
    } catch (const std::exception &e) {
      std::cerr << "Failed to get response: " << e.what() << std::endl;
      return "";
    }
  }
};

#endif // QUERY_GENERATOR_HPP
