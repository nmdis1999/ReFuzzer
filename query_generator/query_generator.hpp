#ifndef QUERY_GENERATOR_HPP
#define QUERY_GENERATOR_HPP

#include <curl/curl.h>
#include <iostream>
#include <string>
#include <stdexcept>

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

  std::string format_response(const std::string &response) {
    std::string formatted = response;
    size_t pos = 0;
    while ((pos = formatted.find("\\n", pos)) != std::string::npos) {
      formatted.replace(pos, 2, "\n");
      pos += 1;
    }
    return formatted;
  }

  std::string extract_response(const std::string &json_response) {
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
    return format_response(extracted_response);
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

  std::string askModel(const std::string &prompt) {
    try {
      if (!curl) {
        throw std::runtime_error("CURL not initialized");
      }

      std::string response_string;
      std::string json_body = "{\"model\":\"" + OLLAMA_MODEL +
                              "\", \"prompt\":\"" + prompt +
                              "\", \"stream\":false}";

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
