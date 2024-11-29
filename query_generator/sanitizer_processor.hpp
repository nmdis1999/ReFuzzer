#ifndef SANITIZER_PROCESSOR_HPP
#define SANITIZER_PROCESSOR_HPP

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <array>
#include <sstream>
#include <cstdio>
namespace fs = std::filesystem;

class SanitizerProcessor {
private:
    bool createDirectory(const std::string& path) {
        try {
            if (!fs::exists(path)) {
                fs::create_directories(path);
                std::cout << "Created directory: " << path << std::endl;
            }
            return true;
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error creating directory " << path << ": " << e.what() << std::endl;
            return false;
        }
    }

    std::string getFileName(const std::string& objectPath) {
        fs::path path(objectPath);
        return path.filename().string();
    }

    bool executeCommand(const std::string& command, std::string& output) {
        std::array<char, 128> buffer;
        output.clear();
        
        FILE* pipe = popen((command + " 2>&1").c_str(), "r");
        if (!pipe) {
            std::cerr << "Error executing command" << std::endl;
            return false;
        }

        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            output += buffer.data();
        }

        return pclose(pipe) == 0;
    }

    void logError(const std::string& objectFile, const std::string& error) {
        if (!createDirectory("../sanitizer_log")) {
            return;
        }

        std::string logFile = "../sanitizer_log/" + getFileName(objectFile) + ".log";

        std::ofstream log(logFile);
        if (log.is_open()) {
            log << "Object File: " << objectFile << "\n";
            log << "Error Output:\n" << error << "\n";
            log.close();
            std::cout << "Error log written to: " << logFile << std::endl;
        }
    }

public:
    void processObjectFiles() {
        if (!createDirectory("../test")) {
            return;
        }

        for (const auto& entry : fs::directory_iterator("../objects")) {
            if (entry.path().extension() == ".o") {
                std::string objectPath = entry.path().string();
                std::string executablePath = "../test/" + getFileName(objectPath);
                
                std::string command = "gcc -fsanitize=address,undefined -fno-omit-frame-pointer " +
                                    objectPath + " -o " + executablePath;

                std::string output;
                bool success = executeCommand(command, output);

                if (!success) {
                    std::cout << "Sanitizer check failed for: " << objectPath << std::endl;
                    logError(objectPath, output);
                } else {
                    std::cout << "Sanitizer check passed for: " << objectPath << "\n";
                    std::cout << "Executable created at: " << executablePath << std::endl;
                }
            }
        }
    }
};

#endif
