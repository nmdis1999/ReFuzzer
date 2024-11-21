#ifndef QUERY_GENERATOR_HPP
#define QUERY_GENERATOR_HPP

#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <string>

using json = nlohmann::json;

class QueryGenerator {
private:
  const std::string OLLAMA_MODEL;
  const std::string base_url;
  CURL *curl;

  static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                              void *userp) {
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
  }

  std::string escapeJsonString(const std::string &input) {
    return json(input).dump().substr(1, std::string::npos - 2);
  }

public:
  QueryGenerator(const std::string &model_name,
                 const std::string &host = "localhost", int port = 11434)
      : OLLAMA_MODEL(model_name),
        base_url("http://" + host + ":" + std::to_string(port)) {
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
      json request = {{"model", OLLAMA_MODEL}};

      std::string response_string;
      std::string url = base_url + "/api/pull";

      struct curl_slist *headers = NULL;
      headers = curl_slist_append(headers, "Content-Type: application/json");

      std::string request_body = request.dump();

      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request_body.length());
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

      json request = {
          {"model", OLLAMA_MODEL}, {"prompt", prompt}, {"stream", false}};

      std::string response_string;
      std::string url = base_url + "/api/generate";
      std::string request_body = request.dump();

      std::cout << "Sending JSON request:" << std::endl;
      std::cout << request_body << std::endl;

      struct curl_slist *headers = NULL;
      headers = curl_slist_append(headers, "Content-Type: application/json");

      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request_body.length());
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

      CURLcode res = curl_easy_perform(curl);
      curl_slist_free_all(headers);

      if (res != CURLE_OK) {
        throw std::runtime_error(std::string("Failed to get response: ") +
                                 curl_easy_strerror(res));
      }

      json response_json = json::parse(response_string);
      std::cout << "Raw JSON response:" << std::endl;
      std::cout << response_json.dump(2) << std::endl;

      if (response_json.contains("response")) {
        return response_json["response"].get<std::string>();
      } else {
        throw std::runtime_error("Response doesn't contain 'response' field");
      }

    } catch (const json::parse_error &e) {
      std::cerr << "JSON parsing error: " << e.what() << std::endl;
      return "";
    } catch (const std::exception &e) {
      std::cerr << "Failed to get response: " << e.what() << std::endl;
      return "";
    }
  }
};

#endif // QUERY_GENERATOR_HPP
