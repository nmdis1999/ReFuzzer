#include "../query_generator/Parser.hpp"
#include "../query_generator/TestWriter.hpp"
#include "../query_generator/object_generator.hpp"
#include "../query_generator/query_generator.hpp"
#include "../query_generator/sanitizer_processor.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <set>

namespace fs = std::filesystem;

// Helper function to read file content
std::string readFileContent(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return "";
    }
    
    return std::string((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
}

// Get files with .log extension from directory
std::vector<std::string> getLogFiles(const std::string& directory) {
    std::vector<std::string> files;
    
    if (!fs::exists(directory)) {
        std::cout << "Directory does not exist: " << directory << std::endl;
        return files;
    }
    
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.path().extension() == ".log") {
            std::string baseName = entry.path().stem().string();
            files.push_back(baseName);
        }
    }
    
    return files;
}

// Check if a file has sanitizer errors
bool hasSanitizerErrors(const std::string& baseFilename) {
    const std::vector<std::string> sanitizerNames = {"asan_ubsan", "tsan", "leak"};
    
    for (const auto& sanitizer : sanitizerNames) {
        std::string logFilename = baseFilename + "_" + sanitizer;
        std::string sanitizerLogPath = "../sanitizer_log/" + logFilename + ".log";
        std::string crashLogPath = "../sanitizer_crash/" + logFilename + ".log";
        
        if (fs::exists(sanitizerLogPath) || fs::exists(crashLogPath)) {
            return true;
        }
    }
    
    return false;
}

// Get model name from command line
std::string parseModelOption(int argc, char *argv[]) {
    std::string defaultModel = "llama3.2";
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.find("--model=") == 0) {
            return arg.substr(8);
        }
    }
    
    return defaultModel;
}

// Main function
int main(int argc, char *argv[]) {
    // Parse model name from command line
    std::string modelName = parseModelOption(argc, argv);
    std::cout << "Using LLM model: " << modelName << std::endl;
    
    // Ensure directories exist
    fs::create_directories("../log");
    fs::create_directories("../test");
    fs::create_directories("../object");
    fs::create_directories("../sanitizer_log");
    fs::create_directories("../sanitizer_crash");
    fs::create_directories("../correct_code");
    
    // Step 1: Get files with compilation errors
    std::vector<std::string> compilationErrorFiles = getLogFiles("../log");
    std::cout << "Found " << compilationErrorFiles.size() << " files with compilation errors" << std::endl;
    
    // Step 2: Get files with sanitizer errors
    std::set<std::string> sanitizerBaseNames;
    
    // Check sanitizer_log and sanitizer_crash directories
    for (const auto& dir : {"../sanitizer_log", "../sanitizer_crash"}) {
        if (!fs::exists(dir)) continue;
        
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (entry.path().extension() == ".log") {
                std::string filename = entry.path().stem().string();
                
                // Extract base filename by removing sanitizer suffix
                const std::vector<std::string> sanitizerNames = {"asan_ubsan", "tsan", "leak"};
                for (const auto& sanitizer : sanitizerNames) {
                    std::string suffix = "_" + sanitizer;
                    if (filename.length() > suffix.length() && 
                        filename.substr(filename.length() - suffix.length()) == suffix) {
                        std::string baseFilename = filename.substr(0, filename.length() - suffix.length());
                        sanitizerBaseNames.insert(baseFilename);
                        break;
                    }
                }
            }
        }
    }
    
    std::vector<std::string> sanitizerErrorFiles(sanitizerBaseNames.begin(), sanitizerBaseNames.end());
    std::cout << "Found " << sanitizerErrorFiles.size() << " files with sanitizer errors" << std::endl;
    
    // Step 3: Combine both lists (avoid duplicates)
    std::set<std::string> allFilesSet;
    for (const auto& file : compilationErrorFiles) allFilesSet.insert(file);
    for (const auto& file : sanitizerErrorFiles) allFilesSet.insert(file);
    
    std::vector<std::string> allFiles(allFilesSet.begin(), allFilesSet.end());
    std::cout << "Total " << allFiles.size() << " unique files to process" << std::endl;
    
    if (allFiles.empty()) {
        std::cout << "No files to process. Exiting." << std::endl;
        return 0;
    }
    
    // Step 4: Initialize LLM model for querying
    QueryGenerator qGenerate(modelName);
    qGenerate.loadModel();
    std::cout << "Model loaded successfully" << std::endl;
    
    // Step 5: Process each file
    for (const auto& file : allFiles) {
        std::cout << "\nProcessing file: " << file << std::endl;
        
        // Check for program file existence
        std::string programPath = "../test/" + file + ".c";
        if (!fs::exists(programPath)) {
            std::cerr << "Program file not found: " << programPath << std::endl;
            continue;
        }
        
        std::string filepath = programPath;
        std::string compile_cmd = "gcc -Wall -Wextra -o " + file + " " + filepath;
        
        // Step 5.1: Fix compilation errors if they exist
        std::string logPath = "../log/" + file + ".log";
        if (fs::exists(logPath)) {
            std::cout << "Found compilation error log, generating fix..." << std::endl;
            
            std::string basePrompt =
                "Given the following C program and its compilation error log with -O0 "
                "optimization level, please analyze and correct the program to resolve "
                "the compilation errors.";
            
            std::string fullPrompt = qGenerate.generatePromptWithFiles(basePrompt, programPath, logPath);
            std::string response = qGenerate.askModel(fullPrompt);
            
            if (response.empty()) {
                std::cerr << "No response generated for compilation errors. Skipping file." << std::endl;
                continue;
            }
            
            Parser parser;
            auto [parsedFilepath, formatSuccess] = parser.parseAndSaveProgram(response, file);
            filepath = parsedFilepath;
            compile_cmd = parser.getGccCommand(response);
            
            std::cout << "Fixed compilation errors and saved to: " << filepath << std::endl;
        }
        
        // Step 5.2: Generate object file
        std::cout << "Generating object file..." << std::endl;
        GenerateObject object;
        std::string objectPath = object.generateObjectFile(filepath, compile_cmd);
        
        if (objectPath.empty()) {
            std::cerr << "Failed to generate object file for " << file << ". Skipping file." << std::endl;
            continue;
        }
        
        std::cout << "Successfully generated object file: " << objectPath << std::endl;
        
        // Step 5.3: Run sanitizer checks
        std::cout << "Running sanitizer checks..." << std::endl;
        SanitizerProcessor sanitizer;
        sanitizer.processSourceFile(objectPath);
        
        // Step 5.4: Check if sanitizer errors still exist
        if (hasSanitizerErrors(file)) {
            std::cout << "Sanitizer errors found, generating fix..." << std::endl;
            
            // Collect sanitizer log content
            std::string sanitizerContent;
            const std::vector<std::string> sanitizerNames = {"asan_ubsan", "tsan", "leak"};
            
            for (const auto& sanitizer : sanitizerNames) {
                std::string logFilename = file + "_" + sanitizer;
                std::string sanitizerLogPath = "../sanitizer_log/" + logFilename + ".log";
                std::string crashLogPath = "../sanitizer_crash/" + logFilename + ".log";
                
                if (fs::exists(sanitizerLogPath)) {
                    std::string logContent = readFileContent(sanitizerLogPath);
                    if (!logContent.empty()) {
                        sanitizerContent += "=== " + sanitizer + " Sanitizer Log ===\n";
                        sanitizerContent += logContent + "\n\n";
                    }
                }
                
                if (fs::exists(crashLogPath)) {
                    std::string crashContent = readFileContent(crashLogPath);
                    if (!crashContent.empty()) {
                        sanitizerContent += "=== " + sanitizer + " Crash Log ===\n";
                        sanitizerContent += crashContent + "\n\n";
                    }
                }
            }
            
            if (!sanitizerContent.empty()) {
                // Generate prompt for sanitizer fix
                std::string programContent = readFileContent(filepath);
                std::string sanitizerPrompt =
                    "Given the following C program and its sanitizer error log with -O0 "
                    "optimization level, please analyze and correct the program to resolve "
                    "the sanitizer errors. Fix all array bounds issues, memory leaks, and "
                    "undefined behavior. Ensure that all memory accesses are within bounds "
                    "of allocated memory.";
                
                std::string fullSanitizerPrompt = sanitizerPrompt + 
                    "\n\nC Program:\n```c\n" + programContent + 
                    "\n```\n\nSanitizer Error Logs:\n```\n" + sanitizerContent + 
                    "\n```\n\nPlease provide the corrected C program:";
                
                std::cout << "Sending prompt to model..." << std::endl;
                std::string sanitizerResponse = qGenerate.askModel(fullSanitizerPrompt);
                
                if (!sanitizerResponse.empty()) {
                    std::cout << "Received fix from model, applying..." << std::endl;
                    
                    Parser parser;
                    auto [newFilepath, newFormatSuccess] = parser.parseAndSaveProgram(sanitizerResponse, file);
                    
                    if (!newFilepath.empty()) {
                        std::string newObjectPath = object.generateObjectFile(newFilepath, compile_cmd);
                        
                        if (!newObjectPath.empty()) {
                            std::cout << "Testing fixed code..." << std::endl;
                            sanitizer.processSourceFile(newObjectPath);
                            
                            if (!hasSanitizerErrors(file)) {
                                std::cout << "All sanitizer errors fixed for " << file << std::endl;
                                
                                // Clean up sanitizer log files
                                for (const auto& sanitizer : sanitizerNames) {
                                    std::string logFilename = file + "_" + sanitizer;
                                    std::string sanitizerLogPath = "../sanitizer_log/" + logFilename + ".log";
                                    std::string crashLogPath = "../sanitizer_crash/" + logFilename + ".log";
                                    
                                    if (fs::exists(sanitizerLogPath)) {
                                        fs::remove(sanitizerLogPath);
                                    }
                                    if (fs::exists(crashLogPath)) {
                                        fs::remove(crashLogPath);
                                    }
                                }
                            } else {
                                std::cerr << "Sanitizer errors still exist after fix attempt for " << file << std::endl;
                            }
                        } else {
                            std::cerr << "Failed to compile fixed code for " << file << std::endl;
                        }
                    } else {
                        std::cerr << "Failed to parse and save fixed program for " << file << std::endl;
                    }
                } else {
                    std::cerr << "No response generated for sanitizer fix for " << file << std::endl;
                }
            } else {
                std::cerr << "No sanitizer content found for " << file << std::endl;
            }
        } else {
            std::cout << "No sanitizer errors found for " << file << std::endl;
            
            // Copy to correct_code directory since it passed all checks
            try {
                fs::path destPath = "../correct_code/" + fs::path(filepath).filename().string();
                fs::copy_file(filepath, destPath, fs::copy_options::overwrite_existing);
                std::cout << "Copied " << filepath << " to " << destPath << std::endl;
            } catch (const fs::filesystem_error& e) {
                std::cerr << "Error copying file to correct_code: " << e.what() << std::endl;
            }
        }
    }
    
    std::cout << "\nProgram completed successfully" << std::endl;
    return 0;
}