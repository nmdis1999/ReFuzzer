#ifndef COMPILER_FIXER_HPP
#define COMPILER_FIXER_HPP

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <sstream>
#include <cstdio>
#include "query_generator.hpp"
#include "TestWriter.hpp"
#include "object_generator.hpp"
#include "Parser.hpp"

namespace fs = std::filesystem;

class CompilerFixer {
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

    std::string getFileName(const std::string& filePath) {
        fs::path path(filePath);
        return path.stem().string();
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

    std::string readFile(const std::string& filePath) {
        try {
            std::ifstream file(filePath);
            if (!file.is_open()) {
                std::cerr << "Failed to open file: " << filePath << std::endl;
                return "";
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        } catch (const std::exception& e) {
            std::cerr << "Error reading file " << filePath << ": " << e.what() << std::endl;
            return "";
        }
    }

    bool writeFile(const std::string& filePath, const std::string& content) {
        try {
            std::ofstream file(filePath, std::ios::out | std::ios::trunc);
            if (!file.is_open()) {
                std::cerr << "Failed to open file for writing: " << filePath << std::endl;
                return false;
            }

            file << content;
            file.close();
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error writing to file " << filePath << ": " << e.what() << std::endl;
            return false;
        }
    }

    // More robust fallback code extractor
    std::string extractCCodeFromResponse(const std::string& response) {
        // First try using Parser's getCProgram method
        std::string extractedCode = Parser::getCProgram(response);
        if (!extractedCode.empty()) {
            return extractedCode;
        }
        
        // Try manual extraction if Parser failed
        
        // Try to find code between ```c and ``` marks
        size_t codeStartMarker = response.find("```c");
        if (codeStartMarker != std::string::npos) {
            size_t codeStart = response.find('\n', codeStartMarker);
            if (codeStart != std::string::npos) {
                codeStart++; // Move past the newline
                size_t codeEnd = response.find("```", codeStart);
                if (codeEnd != std::string::npos) {
                    return response.substr(codeStart, codeEnd - codeStart);
                }
            }
        }
        
        // Try with just ``` marks
        codeStartMarker = response.find("```");
        if (codeStartMarker != std::string::npos) {
            size_t codeStart = response.find('\n', codeStartMarker);
            if (codeStart != std::string::npos) {
                codeStart++; // Move past the newline
                size_t codeEnd = response.find("```", codeStart);
                if (codeEnd != std::string::npos) {
                    return response.substr(codeStart, codeEnd - codeStart);
                }
            }
        }
        
        // Look for typical C code markers
        size_t includePos = response.find("#include");
        if (includePos != std::string::npos) {
            return response.substr(includePos);
        }
        
        size_t intMainPos = response.find("int main");
        if (intMainPos != std::string::npos) {
            return response.substr(intMainPos);
        }
        
        // Look for "Here is the fixed code" or similar markers
        std::vector<std::string> markers = {"fixed code", "corrected code", "Sure!", "Here's", "Here is"};
        for (const auto& marker : markers) {
            size_t markerPos = response.find(marker);
            if (markerPos != std::string::npos) {
                size_t nextLinePos = response.find("\n", markerPos);
                if (nextLinePos != std::string::npos) {
                    std::string afterMarker = response.substr(nextLinePos + 1);
                    // Try to find C code in what's after the marker
                    size_t codePos = afterMarker.find("int main");
                    if (codePos != std::string::npos) {
                        return afterMarker.substr(codePos);
                    }
                    // If we still couldn't find the main function, just return everything after the marker
                    return afterMarker;
                }
            }
        }
        
        // If we still haven't found anything, just return the raw response
        return response;
    }

    std::string getFixedCodeFromLLM(const std::string& sourceCode, const std::string& compileError) {
        // Analyze the sourceCode to check for includes
        bool hasIncludes = sourceCode.find("#include") != std::string::npos;
        bool hasMain = sourceCode.find("int main") != std::string::npos;
        
        std::string prompt = 
            "I have a C program with compilation errors. Please fix the code and return a COMPLETE, COMPILABLE C program.\n\n"
            "Here's the original code:\n```c\n" + sourceCode + "\n```\n\n"
            "Here's the compilation error:\n```\n" + compileError + "\n```\n\n";
            
        // Add specific instructions based on what's missing
        if (!hasIncludes) {
            prompt += "Make sure to include ALL necessary header files (#include statements) in your fixed code. ";
        }
        if (!hasMain) {
            prompt += "Ensure the fixed code has a complete main function. ";
        }
        
        prompt += "Your fixed code MUST include ALL necessary headers, standard library includes, function definitions, and a complete main function.\n\n"
                  "Return the COMPLETE fixed C program without ANY explanations or comments before or after the code.\n```c";

        std::cout << "Sending enhanced request to LLM..." << std::endl;
        try {
            QueryGenerator qGenerate("llama2");
            qGenerate.loadModel();
            return qGenerate.askModel(prompt);
        } catch (const std::exception& e) {
            std::cerr << "Error while getting fixed code from LLM: " << e.what() << std::endl;
            return "";
        }
    }

    // Custom saveFile method that doesn't rely on Parser or TestWriter
    std::string saveFixedCode(const std::string& code, const std::string& fileName) {
        std::string fixedPath = "../fixed_code/" + fileName + ".c";
        if (writeFile(fixedPath, code)) {
            return fixedPath;
        }
        return "";
    }

    bool compileFile(const std::string& sourcePath, std::string& output, bool generateObject = true) {
        std::string command;
        if (generateObject) {
            std::string objectPath = "../object/" + getFileName(sourcePath) + ".o";
            command = "gcc -c " + sourcePath + " -o " + objectPath + " -w"; // Add -w to suppress warnings
        } else {
            // Just check if it compiles without generating object file
            command = "gcc -fsyntax-only " + sourcePath + " -w"; // Add -w to suppress warnings
        }

        std::cout << "Executing: " << command << std::endl;
        return executeCommand(command, output);
    }

    std::string getObjectPathForSource(const std::string& sourcePath) {
        return "../object/" + getFileName(sourcePath) + ".o";
    }
    
    std::string analyzeCompilationErrors(const std::string& errorOutput) {
        std::stringstream analysis;
        
        if (errorOutput.find("undefined reference") != std::string::npos) {
            analysis << "- Missing function implementation or library\n";
        }
        
        if (errorOutput.find("implicit declaration") != std::string::npos) {
            analysis << "- Missing header file include\n";
        }
        
        if (errorOutput.find("unknown type") != std::string::npos || 
            errorOutput.find("undeclared") != std::string::npos) {
            analysis << "- Undefined variable, type, or missing include file\n";
        }
        
        // Add more error patterns as needed
        
        return analysis.str();
    }

public:
    void processDirectory(const std::string& dirPath) {
        // Create necessary directories
        for (const auto& dir : {"../fixed_code", "../compile_errors", "../object", "../test", "../original_files"}) {
            createDirectory(dir);
        }
        
        std::cout << "Starting to process C files in: " << dirPath << std::endl;
        
        // Process each .c file in the directory
        for (const auto& entry : fs::directory_iterator(dirPath)) {
            if (entry.path().extension() == ".c") {
                processFile(entry.path().string());
            }
        }
    }

    void processFile(const std::string& sourcePath) {
        std::string fileName = getFileName(sourcePath);
        std::string objectPath = getObjectPathForSource(sourcePath);
        
        // Skip if object file already exists
        if (fs::exists(objectPath)) {
            std::cout << "Object file already exists for " << fileName << ", skipping." << std::endl;
            return;
        }
        
        std::cout << "\n===============================================" << std::endl;
        std::cout << "Processing: " << sourcePath << std::endl;
        
        // Read the source code first so we have it for the error logs
        std::string sourceCode = readFile(sourcePath);
        if (sourceCode.empty()) {
            std::cerr << "Failed to read source file: " << sourcePath << std::endl;
            return;
        }
        
        // Save the original file for reference
        std::string originalFilePath = "../original_files/" + fileName + ".c";
        writeFile(originalFilePath, sourceCode);
        
        // Try to compile the source file
        std::string compileOutput;
        bool compileSuccess = compileFile(sourcePath, compileOutput);
        
        if (compileSuccess) {
            std::cout << "Compilation successful for: " << sourcePath << std::endl;
            
            // If it compiles successfully, copy to test directory and generate object file
            try {
                // Copy to test directory
                std::string testFilePath = "../test/" + fs::path(sourcePath).filename().string();
                fs::copy_file(sourcePath, testFilePath, fs::copy_options::overwrite_existing);
                std::cout << "Copied to test directory: " << testFilePath << std::endl;
                
                // Generate object file
                GenerateObject objectGenerator;
                std::string compileCmd = "gcc -c " + testFilePath;
                std::string objPath = objectGenerator.generateObjectFile(testFilePath, compileCmd);
                if (!objPath.empty()) {
                    std::cout << "Created object file: " << objPath << std::endl;
                }
            } catch (const fs::filesystem_error& e) {
                std::cerr << "Error copying file: " << e.what() << std::endl;
            }
            return;
        }
        
        // Compilation failed, analyze errors and save to log
        std::string errorAnalysis = analyzeCompilationErrors(compileOutput);
        std::cout << "Error analysis:\n" << errorAnalysis << std::endl;
        
        std::string errorFilePath = "../compile_errors/" + fileName + ".txt";
        // Write both the source code, error analysis, and error to the log
        std::string fullErrorLog = "ORIGINAL SOURCE CODE:\n\n" + sourceCode + "\n\n";
        fullErrorLog += "ERROR ANALYSIS:\n\n" + errorAnalysis + "\n\n";
        fullErrorLog += "COMPILATION ERROR:\n\n" + compileOutput;
        writeFile(errorFilePath, fullErrorLog);
        std::cout << "Compilation failed. Error saved to: " << errorFilePath << std::endl;
        
        // Get fixed code from LLM
        std::cout << "Requesting fixed code from LLM for: " << fileName << std::endl;
        std::string response = getFixedCodeFromLLM(sourceCode, compileOutput);
        
        if (response.empty()) {
            std::cerr << "Failed to get fixed code from LLM for: " << fileName << std::endl;
            return;
        }
        
        // Extract the fixed code with our custom extractor
        std::string fixedCode = extractCCodeFromResponse(response);
        
        if (fixedCode.empty()) {
            std::cerr << "Failed to extract fixed code from LLM response" << std::endl;
            // Save the raw response for debugging
            writeFile("../compile_errors/" + fileName + "_raw_response.txt", response);
            return;
        }
        
        // Check if the fixed code has includes and main function
        bool hasIncludes = fixedCode.find("#include") != std::string::npos;
        bool hasMain = fixedCode.find("int main") != std::string::npos;
        
        if (!hasIncludes || !hasMain) {
            std::cout << "Warning: Fixed code appears incomplete. " << 
                (!hasIncludes ? "Missing includes. " : "") << 
                (!hasMain ? "Missing main function." : "") << std::endl;
        }
        
        std::cout << "============================" << std::endl;
        std::cout << "Extracted fixed code: " << std::endl;
        std::cout << fixedCode << std::endl;
        std::cout << "============================" << std::endl;
        
        // Save the fixed code
        std::string fixedPath = saveFixedCode(fixedCode, fileName);
        if (fixedPath.empty()) {
            std::cerr << "Failed to save fixed code" << std::endl;
            return;
        }
        
        std::cout << "Fixed code saved to: " << fixedPath << std::endl;
        
        // Try to compile the fixed code
        std::string fixedCompileOutput;
        bool fixedCompileSuccess = compileFile(fixedPath, fixedCompileOutput, false);
        
        if (fixedCompileSuccess) {
            std::cout << "Fixed code compilation successful for: " << fileName << std::endl;
            
            // Copy successful fixed file to test directory
            std::string testFilePath = "../test/" + fileName + ".c";
            try {
                fs::copy_file(fixedPath, testFilePath, fs::copy_options::overwrite_existing);
                std::cout << "Fixed code copied to test directory: " << testFilePath << std::endl;
                
                // Generate object file from fixed code
                GenerateObject objectGenerator;
                std::string compileCmd = "gcc -c " + testFilePath;
                std::string objPath = objectGenerator.generateObjectFile(testFilePath, compileCmd);
                if (!objPath.empty()) {
                    std::cout << "Created object file: " << objPath << std::endl;
                }
            } catch (const fs::filesystem_error& e) {
                std::cerr << "Error copying fixed file to test directory: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "Fixed code still has compilation errors for: " << fileName << std::endl;
            
            // Try a second attempt with more specific instructions
            std::cout << "Making a second attempt with more specific instructions..." << std::endl;
            std::string secondPrompt = 
                "The previous fixed code still doesn't compile. Please fix this C program again, ensuring it is COMPLETE and COMPILABLE.\n\n"
                "Original code:\n```c\n" + sourceCode + "\n```\n\n"
                "New compilation errors:\n```\n" + fixedCompileOutput + "\n```\n\n"
                "Your response must be a COMPLETE C program with ALL necessary #include statements, function definitions, and a main function.\n"
                "DO NOT omit any standard headers. Include ALL required libraries (stdio.h, stdlib.h, etc.).\n"
                "Return ONLY the complete fixed C code with no explanations:\n```c";
            QueryGenerator qGenerate("llama2");
            qGenerate.loadModel();
            std::string secondResponse = qGenerate.askModel(secondPrompt);
            std::string secondFixedCode = extractCCodeFromResponse(secondResponse);
            
            if (!secondFixedCode.empty()) {
                std::string secondFixedPath = saveFixedCode(secondFixedCode, fileName + "_attempt2");
                std::cout << "Second attempt saved to: " << secondFixedPath << std::endl;
                
                std::string secondCompileOutput;
                bool secondCompileSuccess = compileFile(secondFixedPath, secondCompileOutput, false);
                
                if (secondCompileSuccess) {
                    std::cout << "Second attempt compilation successful!" << std::endl;
                    // Copy to test directory and generate object file
                    fs::copy_file(secondFixedPath, "../test/" + fileName + ".c", fs::copy_options::overwrite_existing);
                    GenerateObject().generateObjectFile("../test/" + fileName + ".c", "gcc -c ../test/" + fileName + ".c");
                    return;
                }
            }
            
            // If we're here, both attempts failed
            std::string fixedErrorPath = "../compile_errors/" + fileName + "_fixed_error.txt";
            // Write both the fixed code and the error to the log
            std::string fullFixedErrorLog = "FIXED SOURCE CODE:\n\n" + fixedCode + "\n\n";
            fullFixedErrorLog += "COMPILATION ERROR:\n\n" + fixedCompileOutput;
            writeFile(fixedErrorPath, fullFixedErrorLog);
        }
        std::cout << "===============================================\n" << std::endl;
    }
};

#endif