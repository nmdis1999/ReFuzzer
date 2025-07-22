#include "../query_generator/Parser.hpp"
#include "../query_generator/query_generator.hpp"
#include "../query_generator/object_generator.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Read file content
std::string readFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return "";
    }
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

// Parse command line options
struct Options {
    std::string dir = "";
    std::string model = "llama3.2";
    std::string compileLogDir = "";
    std::string sanitizeLogDir = "";
};

Options parseArgs(int argc, char* argv[]) {
    Options opts;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg.find("--dir=") == 0) {
            opts.dir = arg.substr(6);
        } else if (arg.find("--model=") == 0) {
            opts.model = arg.substr(8);
        } else if (arg.find("--compile=") == 0) {
            opts.compileLogDir = arg.substr(10);
        } else if (arg.find("--sanitize=") == 0) {
            opts.sanitizeLogDir = arg.substr(11);
        }
    }
    
    return opts;
}

// Fix compilation errors
bool fixCompilationError(const std::string& sourceFile, const std::string& logFile, 
                        const std::string& model, const std::string& dir, const std::string& logDir) {
    std::cout << "  Fixing compilation error..." << std::endl;
    
    std::string sourceCode = readFile(sourceFile);
    std::string errorLog = readFile(logDir + "/" + logFile);
    
    if (sourceCode.empty() || errorLog.empty()) {
        std::cout << "  Failed to read source or log file" << std::endl;
        return false;
    }
    
    // Create prompt
    std::string prompt = 
        "Fix the following C++ compilation error. Return only the corrected C++ code, no explanations.\n\n"
        "Source code:\n```cpp\n" + sourceCode + "\n```\n\n"
        "Compilation error:\n```\n" + errorLog + "\n```\n\n"
        "Corrected code:";
    
    // Get fix from LLM
    QueryGenerator qGen(model);
    qGen.loadModel();
    std::string response = qGen.askModel(prompt);
    
    if (response.empty()) {
        std::cout << "  No response from LLM" << std::endl;
        return false;
    }
    
    // Parse and save fixed code
    auto [fixedPath, success] = Parser::parseAndSaveProgram(response, "", dir);
    if (!success || fixedPath.empty()) {
        std::cout << "  Failed to parse fixed code" << std::endl;
        return false;
    }
    
    // Try to compile fixed code
    GenerateObject objGen;
    std::string objectPath = objGen.generateObjectFile(fixedPath, dir);
    
    if (!objectPath.empty()) {
        std::cout << "  ✓ Compilation error fixed!" << std::endl;
        fs::remove(logDir + "/" + logFile);
        // Move to correct directory in original dir
        std::string basename = fs::path(fixedPath).filename().string();
        std::string correctPath = dir + "/correct/" + basename;
        fs::create_directories(dir + "/correct");
        fs::copy_file(fixedPath, correctPath, fs::copy_options::overwrite_existing);
        return true;
    } else {
        std::cout << "  ✗ Fix attempt failed" << std::endl;
        return false;
    }
}

// Fix sanitizer errors
bool fixSanitizerError(const std::string& sourceFile, const std::string& logFile,
                      const std::string& model, const std::string& dir, const std::string& logDir) {
    std::cout << "  Fixing sanitizer error..." << std::endl;
    
    std::string sourceCode = readFile(sourceFile);
    std::string errorLog = readFile(logDir + "/" + logFile);
    
    if (sourceCode.empty() || errorLog.empty()) {
        std::cout << "  Failed to read source or log file" << std::endl;
        return false;
    }
    
    // Create prompt
    std::string prompt = 
        "Fix the following C++ sanitizer errors. Return only the corrected C++ code, no explanations.\n\n"
        "Source code:\n```cpp\n" + sourceCode + "\n```\n\n"
        "Sanitizer errors:\n```\n" + errorLog + "\n```\n\n"
        "Corrected code:";
    
    // Get fix from LLM
    QueryGenerator qGen(model);
    qGen.loadModel();
    std::string response = qGen.askModel(prompt);
    
    if (response.empty()) {
        std::cout << "  No response from LLM" << std::endl;
        return false;
    }
    
    // Parse and save fixed code
    auto [fixedPath, success] = Parser::parseAndSaveProgram(response, "", dir);
    if (!success || fixedPath.empty()) {
        std::cout << "  Failed to parse fixed code" << std::endl;
        return false;
    }
    
    // Try to compile and test fixed code
    GenerateObject objGen;
    std::string objectPath = objGen.generateObjectFile(fixedPath, dir);
    
    if (!objectPath.empty()) {
        std::cout << "  ✓ Sanitizer error fixed!" << std::endl;
        fs::remove(logDir + "/" + logFile);
        // Move to correct directory in original dir
        std::string basename = fs::path(fixedPath).filename().string();
        std::string correctPath = dir + "/correct/" + basename;
        fs::create_directories(dir + "/correct");
        fs::copy_file(fixedPath, correctPath, fs::copy_options::overwrite_existing);
        return true;
    } else {
        std::cout << "  ✗ Fix attempt failed" << std::endl;
        return false;
    }
}

void displayHelp() {
    std::cout << "Usage: ./recompile --dir=<directory> --compile=<path> --sanitize=<path> [--model=<model>]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --dir=<path>        Directory containing .cpp files\n";
    std::cout << "  --compile=<path>    Fix compilation errors from specified log directory\n";
    std::cout << "  --sanitize=<path>   Fix sanitizer errors from specified log directory\n";
    std::cout << "  --model=<name>      Ollama model to use (default: llama3.2)\n";
    std::cout << "\nExamples:\n";
    std::cout << "  ./recompile --dir=~/test --compile=~/logs/compilation\n";
    std::cout << "  ./recompile --dir=~/test --sanitize=~/logs/sanitizer\n";
    std::cout << "  ./recompile --dir=~/test --compile=~/logs/compilation --sanitize=~/logs/sanitizer --model=llama3\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        displayHelp();
        return 1;
    }
    
    Options opts = parseArgs(argc, argv);
    
    if (opts.dir.empty()) {
        std::cerr << "Error: --dir option is required\n";
        displayHelp();
        return 1;
    }
    
    if (opts.compileLogDir.empty() && opts.sanitizeLogDir.empty()) {
        std::cerr << "Error: At least one of --compile=<path> or --sanitize=<path> must be specified\n";
        displayHelp();
        return 1;
    }
    
    if (!fs::exists(opts.dir) || !fs::is_directory(opts.dir)) {
        std::cerr << "Error: " << opts.dir << " is not a valid directory\n";
        return 1;
    }
    
    std::cout << "Refuzzing directory: " << opts.dir << std::endl;
    std::cout << "Using model: " << opts.model << std::endl;
    
    int fixedFiles = 0;
    int totalAttempts = 0;
    
    // Fix compilation errors
    if (!opts.compileLogDir.empty()) {
        std::cout << "\n=== FIXING COMPILATION ERRORS ===" << std::endl;
        std::cout << "Using log directory: " << opts.compileLogDir << std::endl;
        
        if (fs::exists(opts.compileLogDir)) {
            for (const auto& entry : fs::directory_iterator(opts.compileLogDir)) {
                if (entry.path().extension() == ".log") {
                    std::string logFile = entry.path().filename().string();
                    std::string basename = entry.path().stem().string();  // Remove .log extension
                    std::string sourceFile = opts.dir + "/" + basename + ".cpp";  // Add .cpp extension
                    
                    if (fs::exists(sourceFile)) {
                        std::cout << "Processing " << logFile << " -> " << basename << ".cpp..." << std::endl;
                        totalAttempts++;
                        
                        // Try fixing twice
                        bool fixed = false;
                        for (int attempt = 1; attempt <= 2 && !fixed; attempt++) {
                            std::cout << "  Attempt " << attempt << "/2" << std::endl;
                            fixed = fixCompilationError(sourceFile, logFile, opts.model, opts.dir, opts.compileLogDir);
                        }
                        
                        if (fixed) {
                            fixedFiles++;
                        }
                    } else {
                        std::cout << "Warning: Source file not found: " << sourceFile << std::endl;
                    }
                }
            }
        } else {
            std::cout << "Error: Compilation log directory not found: " << opts.compileLogDir << std::endl;
        }
    }
    
    // Fix sanitizer errors
    if (!opts.sanitizeLogDir.empty()) {
        std::cout << "\n=== FIXING SANITIZER ERRORS ===" << std::endl;
        std::cout << "Using log directory: " << opts.sanitizeLogDir << std::endl;
        
        if (fs::exists(opts.sanitizeLogDir)) {
            for (const auto& entry : fs::directory_iterator(opts.sanitizeLogDir)) {
                if (entry.path().extension() == ".log") {
                    std::string logFile = entry.path().filename().string();
                    std::string basename = entry.path().stem().string();  // Remove .log extension
                    std::string sourceFile = opts.dir + "/" + basename + ".cpp";  // Add .cpp extension
                    
                    if (fs::exists(sourceFile)) {
                        std::cout << "Processing " << logFile << " -> " << basename << ".cpp..." << std::endl;
                        totalAttempts++;
                        
                        // Try fixing twice
                        bool fixed = false;
                        for (int attempt = 1; attempt <= 2 && !fixed; attempt++) {
                            std::cout << "  Attempt " << attempt << "/2" << std::endl;
                            fixed = fixSanitizerError(sourceFile, logFile, opts.model, opts.dir, opts.sanitizeLogDir);
                        }
                        
                        if (fixed) {
                            fixedFiles++;
                        }
                    } else {
                        std::cout << "Warning: Source file not found: " << sourceFile << std::endl;
                    }
                }
            }
        } else {
            std::cout << "Error: Sanitizer log directory not found: " << opts.sanitizeLogDir << std::endl;
        }
    }
    
    std::cout << "\n=== SUMMARY ===" << std::endl;
    std::cout << "Total fix attempts: " << totalAttempts << std::endl;
    std::cout << "Successfully fixed: " << fixedFiles << std::endl;
    std::cout << "Fix success rate: " << (totalAttempts > 0 ? (fixedFiles * 100.0 / totalAttempts) : 0) << "%" << std::endl;
    
    return 0;
}