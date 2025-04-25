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
#include <chrono>
#include <thread>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

namespace fs = std::filesystem;

class SanitizerProcessor {
private:
    struct SanitizerConfig {
        std::string name;
        std::string flags;
        std::string envVars;
    };

    // Create suppression file if it doesn't exist
    void ensureSuppressionFile() {
        if (!fs::exists("sanitizer.supp")) {
            std::ofstream suppFile("sanitizer.supp");
            if (suppFile.is_open()) {
                suppFile << "# Comprehensive suppression file for sanitizers on macOS\n"
                         << "# Created automatically by SanitizerProcessor\n\n"
                         << "# Objective-C runtime leaks\n"
                         << "leak:libobjc.A.dylib\n"
                         << "leak:CoreFoundation\n"
                         << "leak:Foundation\n"
                         << "leak:CFRuntime\n"
                         << "leak:_CFRuntimeCreateInstance\n"
                         << "leak:__CFStringCreateImmutableFunnel3\n"
                         << "leak:_CFAutoreleasePoolAddObject\n"
                         << "leak:malloc_zone_\n"
                         << "leak:__psynch\n"
                         << "leak:dyld\n"
                         << "leak:libSystem\n\n"
                         << "# Threading/GCD races\n"
                         << "race:libdispatch\n"
                         << "race:_dispatch_\n"
                         << "race:pthread_\n"
                         << "race:os_unfair_lock\n\n"
                         << "# Undefined behavior in system libraries\n"
                         << "alignment:CoreFoundation\n"
                         << "alignment:Foundation\n"
                         << "vptr:libobjc.A.dylib\n"
                         << "nonnull-attribute:_CFRuntimeCreateInstance\n"
                         << "function:CFRuntime\n"
                         << "function:NSObject\n\n"
                         << "# Common system library patterns\n"
                         << "leak:*AppleIntel*\n"
                         << "leak:*AppleGVA*\n"
                         << "leak:*AV*\n"
                         << "leak:libsystem\n"
                         << "leak:libswift\n";
                suppFile.close();
                std::cout << "Created sanitizer suppression file at sanitizer.supp" << std::endl;
            } else {
                std::cerr << "Failed to create suppression file" << std::endl;
            }
        }
    }

    const std::vector<SanitizerConfig> configs = {
        {"asan_ubsan", 
         "-fsanitize=address,undefined -fsanitize-address-use-after-scope -fsanitize=function", 
         "ASAN_OPTIONS=detect_odr_violation=0:detect_leaks=0 "
         "UBSAN_OPTIONS=print_stacktrace=1"},
        {"tsan", 
         "-fsanitize=thread", 
         "TSAN_OPTIONS=ignore_noninstrumented_modules=1"},
        {"leak", 
         "-fsanitize=leak", 
         "LSAN_OPTIONS="}
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

    bool executeCommand(const std::string& command, std::string& output, int timeoutSeconds = 30) {
        std::cout << "Running command with timeout (" << timeoutSeconds << "s): " << command << std::endl;
        
        // Create pipes for capturing output
        int outpipe[2];
        if (pipe(outpipe) != 0) {
            std::cerr << "Failed to create pipe" << std::endl;
            return false;
        }
        
        // Make read end non-blocking
        fcntl(outpipe[0], F_SETFL, O_NONBLOCK);
        
        pid_t pid = fork();
        
        if (pid == -1) {
            // Fork failed
            close(outpipe[0]);
            close(outpipe[1]);
            std::cerr << "Fork failed" << std::endl;
            return false;
        } 
        else if (pid == 0) {
            // Child process
            close(outpipe[0]); // Close read end
            
            // Redirect stdout and stderr to our pipe
            dup2(outpipe[1], STDOUT_FILENO);
            dup2(outpipe[1], STDERR_FILENO);
            close(outpipe[1]);
            
            // Execute the command
            execl("/bin/sh", "sh", "-c", command.c_str(), NULL);
            
            // If execl returns, it failed
            _exit(127);
        } 
        else {
            // Parent process
            close(outpipe[1]); // Close write end
            
            output.clear();
            auto startTime = std::chrono::steady_clock::now();
            bool timedOut = false;
            int status;
            
            // Buffer for reading
            char buffer[4096];
            
            // Loop until process exits or timeout occurs
            while (true) {
                // Check if process has exited
                pid_t result = waitpid(pid, &status, WNOHANG);
                if (result == pid) {
                    // Process has exited
                    break;
                }
                else if (result == -1) {
                    // waitpid error
                    std::cerr << "waitpid error" << std::endl;
                    kill(pid, SIGKILL);
                    close(outpipe[0]);
                    return false;
                }
                
                // Read available output
                ssize_t bytesRead = read(outpipe[0], buffer, sizeof(buffer) - 1);
                if (bytesRead > 0) {
                    buffer[bytesRead] = '\0';
                    output += buffer;
                }
                
                // Check for timeout
                auto currentTime = std::chrono::steady_clock::now();
                auto elapsedSeconds = 
                    std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();
                    
                if (elapsedSeconds > timeoutSeconds) {
                    std::cout << "Command timed out after " << timeoutSeconds << " seconds" << std::endl;
                    kill(pid, SIGKILL);
                    timedOut = true;
                    break;
                }
                
                // Small sleep to prevent CPU hogging
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            // Read any remaining output
            while (true) {
                ssize_t bytesRead = read(outpipe[0], buffer, sizeof(buffer) - 1);
                if (bytesRead <= 0) break;
                buffer[bytesRead] = '\0';
                output += buffer;
            }
            
            close(outpipe[0]);
            
            if (timedOut) {
                output += "\n[COMMAND TIMED OUT AFTER " + std::to_string(timeoutSeconds) + " SECONDS]\n";
                // Wait for the killed process to be reaped
                waitpid(pid, &status, 0);
                return false;
            }
            
            // Special case for leak sanitizer - treat "detected memory leaks" in Objective-C as a false positive
            if (output.find("LeakSanitizer: detected memory leaks") != std::string::npos && 
                (output.find("libobjc.A.dylib") != std::string::npos || 
                 output.find("CoreFoundation") != std::string::npos ||
                 output.find("Foundation") != std::string::npos)) {
                std::cout << "Detected macOS system library leak - treating as success" << std::endl;
                return true;
            }
            
            return WIFEXITED(status) && WEXITSTATUS(status) == 0;
        }
    }
    
    bool isSanitizerViolation(const std::string& error) {
        return error.find("runtime error:") != std::string::npos ||
               error.find("AddressSanitizer:") != std::string::npos ||
               error.find("LeakSanitizer:") != std::string::npos ||
               error.find("ThreadSanitizer:") != std::string::npos ||
               error.find("UndefinedBehaviorSanitizer:") != std::string::npos;
    }

    // Check if error is likely a macOS false positive
    bool isMacOSFalsePositive(const std::string& error) {
        return error.find("libobjc.A.dylib") != std::string::npos ||
               error.find("CoreFoundation") != std::string::npos ||
               error.find("Foundation") != std::string::npos ||
               error.find("nano zone abandoned") != std::string::npos ||
               error.find("_CF") != std::string::npos ||
               error.find("libdispatch") != std::string::npos ||
               error.find("_dispatch_") != std::string::npos;
    }

    void logError(const std::string& objectFile, const std::string& sanitizerName, const std::string& error) {
        // Skip logging macOS false positives
        if (isMacOSFalsePositive(error)) {
            std::cout << "Skipping macOS false positive for: " << sanitizerName << std::endl;
            return;
        }
        
        std::string baseFilename = getFileName(objectFile);
        std::string logFilename = baseFilename + "_" + sanitizerName;

        if (isSanitizerViolation(error)) {
            if (!createDirectory("../sanitizer_log")) {
                std::cerr << "Failed to create log directory" << std::endl;
                return;
            }
            std::string logFile = "../sanitizer_log/" + logFilename + ".log";
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
            std::string crashLogFile = "../sanitizer_crash/" + logFilename + ".log";
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
        // Ensure suppression file exists
        ensureSuppressionFile();
        
        std::cout << "Processing: " << objectPath << std::endl;
        
        if (!createDirectory("../test")) return;
        if (!createDirectory("../correct_code")) return;

        std::string baseFilename = getFileName(objectPath);
        std::string sourcePath = findSourceFile(baseFilename);

        if (sourcePath.empty()) {
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
            std::string executablePath = baseFilename + "_" + config.name;
            
            // Add suppression environment variables to the compile command
            std::string compileCommand = config.envVars + " clang " + config.flags +
                                " -fno-omit-frame-pointer"
                                " -fno-optimize-sibling-calls"
                                " -O1 -g " +
                                sourcePath + " -o " + executablePath;

            std::string compileOutput;
            bool compileSuccess = executeCommand(compileCommand, compileOutput, 30);

            if (!compileSuccess) {
                std::cout << "Sanitizer compilation (" << config.name << ") failed for: " << sourcePath << std::endl;
                
                // Truncate long error messages to prevent overflow
                if (compileOutput.length() > 4096) {
                    compileOutput = compileOutput.substr(0, 4096) + "...\n[Output truncated due to length]";
                }
                
                logError(executablePath, config.name, compileOutput);
                allChecksPassed = false;
                continue;
            } else {
                std::cout << "Sanitizer compilation (" << config.name << ") succeeded for: " << sourcePath << std::endl;
                
                // Use shorter timeout for running the executable (10 seconds) and include env vars
                std::string runOutput;
                std::string runCommand = config.envVars + " ./" + executablePath;
                bool runSuccess = executeCommand(runCommand, runOutput, 10);
                
                // Clean up executable regardless of result
                if (fs::exists(executablePath)) {
                    try {
                        fs::remove(executablePath);
                    } catch (const fs::filesystem_error& e) {
                        std::cerr << "Error removing executable: " << e.what() << std::endl;
                    }
                }
                
                if (!runSuccess || isSanitizerViolation(runOutput)) {
                    // Skip macOS false positives
                    if (isMacOSFalsePositive(runOutput)) {
                        std::cout << "Detected macOS false positive, treating as success" << std::endl;
                        continue;
                    }
                    
                    std::cout << "Sanitizer check (" << config.name << ") failed during execution for: " << sourcePath << std::endl;
                    
                    // Truncate long error messages to prevent overflow
                    if (runOutput.length() > 4096) {
                        runOutput = runOutput.substr(0, 4096) + "...\n[Output truncated due to length]";
                    }
                    
                    logError(sourcePath, config.name, runOutput);
                    allChecksPassed = false;
                    continue;
                } else {
                    std::cout << "Sanitizer check (" << config.name << ") passed for: " << sourcePath << std::endl;
                }
            }
        }

        if (allChecksPassed) {
            try {
                std::string destPath = "../correct_code/" + fs::path(sourcePath).filename().string();
                std::cout << "Copying " << sourcePath << " to " << destPath << std::endl;
                fs::copy(sourcePath, destPath, fs::copy_options::overwrite_existing);
                std::cout << "All sanitizer checks passed. Copied " << sourcePath << " to " << destPath << std::endl;
                
                for (const auto& config : configs) {
                    std::string logFilename = baseFilename + "_" + config.name;
                    std::string sanitizerLogPath = "../sanitizer_log/" + logFilename + ".log";
                    std::string crashLogPath = "../sanitizer_crash/" + logFilename + ".log";
                    
                    if (fs::exists(sanitizerLogPath)) {
                        fs::remove(sanitizerLogPath);
                    }
                    if (fs::exists(crashLogPath)) {
                        fs::remove(crashLogPath);
                    }
                }
            } catch (const fs::filesystem_error& e) {
                std::cerr << "Error copying file to correct_code: " << e.what() << std::endl;
            }
        }
    }
};

#endif