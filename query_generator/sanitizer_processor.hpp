#ifndef SANITIZER_PROCESSOR_HPP
#define SANITIZER_PROCESSOR_HPP

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <array>
#include <sstream>
#include <cstdio>
#include <vector>
namespace fs = std::filesystem;

class SanitizerProcessor {
private:
    struct SanitizerConfig {
        std::string name;
        std::string flags;
    };

    const std::vector<SanitizerConfig> configs = {
        {"asan_ubsan", "-fsanitize=address,undefined -fsanitize-address-use-after-scope"},
        {"tsan", "-fsanitize=thread"},
        {"leak", "-fsanitize=leak"}
    };

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
        std::string filename = path.filename().string();
        return filename.substr(0, filename.find_last_of('.'));
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

    bool isSanitizerViolation(const std::string& error) {
        return error.find("runtime error:") != std::string::npos ||
               error.find("AddressSanitizer:") != std::string::npos ||
               error.find("LeakSanitizer:") != std::string::npos ||
               error.find("ThreadSanitizer:") != std::string::npos ||
               error.find("UndefinedBehaviorSanitizer:") != std::string::npos;
    }

    void logError(const std::string& objectFile, const std::string& sanitizerName, const std::string& error) {
        std::string baseFilename = getFileName(objectFile);

        if (isSanitizerViolation(error)) {
            if (!createDirectory("../sanitizer_log")) {
                std::cerr << "Failed to create log directory" << std::endl;
                return;
            }
            std::string logFile = "../sanitizer_log/" + baseFilename + ".log";
            std::ofstream log(logFile, std::ios::app);
            if (!log.is_open()) {
                std::cerr << "Failed to open log file: " << logFile << std::endl;
                return;
            }
            log << "\n=== New Sanitizer Violation Report ===\n";
            log << "Object File: " << objectFile << "\n";
            log << "Sanitizer: " << sanitizerName << "\n";
            log << "Sanitizer Violation:\n" << error << "\n";
            log << "=====================================\n";
            log.close();
            std::cout << "Sanitizer violation log appended to: " << logFile << std::endl;
        } else {
            if (!createDirectory("../sanitizer_crash")) {
                std::cerr << "Failed to create crash log directory" << std::endl;
                return;
            }
            std::string crashLogFile = "../sanitizer_crash/" + baseFilename + ".log";
            std::ofstream crashLog(crashLogFile, std::ios::app);
            if (!crashLog.is_open()) {
                std::cerr << "Failed to open crash log file: " << crashLogFile << std::endl;
                return;
            }
            crashLog << "\n=== New Sanitizer Crash Report ===\n";
            crashLog << "Object File: " << objectFile << "\n";
            crashLog << "Sanitizer: " << sanitizerName << "\n";
            crashLog << "Sanitizer Error Output:\n" << error << "\n";
            crashLog << "================================\n";
            crashLog.close();
            std::cout << "Sanitizer crash log appended to: " << crashLogFile << std::endl;
        }
    }

    std::string findSourceFile(const std::string& baseFilename) {
        // Check all possible locations for the source file
        std::vector<std::string> searchPaths = {
            "../test/" + baseFilename + ".c",
            "../source/" + baseFilename + ".c",
            "../" + baseFilename + ".c"
        };

        for (const auto& path : searchPaths) {
            if (fs::exists(path)) {
                return path;
            }
        }
        return "";
    }

public:
    void processSourceFile(const std::string& objectPath) {
        if (!createDirectory("../test")) return;

        std::string baseFilename = getFileName(objectPath);
        std::string sourcePath = findSourceFile(baseFilename);

        if (sourcePath.empty()) {
            // Try to find a similarly named source file
            for (const auto& entry : fs::directory_iterator("../test")) {
                if (entry.path().extension() == ".c" &&
                    entry.path().stem().string().find(baseFilename) != std::string::npos) {
                    sourcePath = entry.path().string();
                    break;
                }
            }
        }

        if (sourcePath.empty()) {
            std::cerr << "Source file not found for object file: " << objectPath << std::endl;
            return;
        }

        std::cout << "Found source file: " << sourcePath << std::endl;

        bool allChecksPassed = true;
        for (const auto& config : configs) {
            std::string executablePath = "../test/" + baseFilename + "_" + config.name;
            std::string command = "clang " + config.flags +
                                " -fno-omit-frame-pointer"
                                " -fno-optimize-sibling-calls"
                                " -O1 -g " +
                                sourcePath + " -o " + executablePath;

            std::string output;
            bool success = executeCommand(command, output);

            if (!success) {
                std::cout << "Sanitizer check (" << config.name << ") failed for: " << sourcePath << std::endl;
                std::cout << "Error output: " << output << std::endl;
                logError(sourcePath, config.name, output);
                allChecksPassed = false;
                break;
            } else {
                std::cout << "Sanitizer check (" << config.name << ") passed for: " << sourcePath << std::endl;

                std::string runOutput;
                if (!executeCommand(executablePath, runOutput) && !runOutput.empty()) {
                    std::cout << "Runtime check (" << config.name << ") failed for: " << sourcePath << std::endl;
                    std::cout << "Runtime output: " << runOutput << std::endl;
                    logError(sourcePath, config.name, runOutput);
                    allChecksPassed = false;
                    break;
                }
            }
        }

        if (allChecksPassed) {
            if (!createDirectory("../correct_code")) {
                std::cerr << "Failed to create correct_code directory" << std::endl;
                return;
            }

            try {
                std::string destPath = "../correct_code/" + fs::path(sourcePath).filename().string();
                std::cout << "Copying " << sourcePath << " to " << destPath << std::endl;
                fs::copy(sourcePath, destPath, fs::copy_options::overwrite_existing);
                std::cout << "All checks passed. Copied " << sourcePath << " to " << destPath << std::endl;
            } catch (const fs::filesystem_error& e) {
                std::cerr << "Error copying file to correct_code: " << e.what() << std::endl;
            }
        }
    }
};

#endif
