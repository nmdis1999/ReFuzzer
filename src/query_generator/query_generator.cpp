#include "query_generator.hpp"
#include "Parser.hpp"
#include "PromptWriter.hpp"
#include "TestWriter.hpp"
#include "differential_tester.hpp"
#include "llm_tokens_options.hpp"
#include "object_generator.hpp"
#include "sanitizer_processor.hpp"
#include "compiler_fixer.hpp"
#include <filesystem>
#include <iostream>
#include <string>

// Include the new code fixer function
void fixCFiles(const std::string& dirPath);

namespace fs = std::filesystem;

// Function to compile a directory of C files
void compileCFilesInDirectory(const std::string& dirPath) {
  // Clean and recreate directories
  if (fs::exists("../test")) {
    fs::remove_all("../test");
  }
  fs::create_directories("../test");
  fs::create_directories("../object");
  
  GenerateObject objectGenerator;
  
  bool foundFiles = false;
  try {
    for (const auto& entry : fs::directory_iterator(dirPath)) {
      if (entry.path().extension() == ".c") {
        foundFiles = true;
        std::cout << "Processing: " << entry.path().string() << std::endl;
        
        // Copy file to ../test directory
        std::string filename = entry.path().filename().string();
        fs::path destPath = "../test/" + filename;
        fs::copy_file(entry.path(), destPath, fs::copy_options::overwrite_existing);
        std::cout << "Copied to: " << destPath.string() << std::endl;
        
        // Default compile command (can be enhanced with options)
        std::string compileCmd = "gcc -c " + destPath.string();
        
        std::string objectPath = objectGenerator.generateObjectFile(destPath.string(), compileCmd);
        if (objectPath.empty()) {
          std::cerr << "Error: failed generating object file for " << destPath.string() << std::endl;
        } else {
          std::cout << "Successfully created object file: " << objectPath << std::endl;
        }
      }
    }
    
    if (!foundFiles) {
      std::cout << "No .c files found in " << dirPath << " directory to process." << std::endl;
    }
  } catch (const fs::filesystem_error& e) {
    std::cerr << "Filesystem error: " << e.what() << std::endl;
  }
}

void displayHelp() {
  std::cout << "Usage: ./program <option> [directory_path]" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  0: Reserved option" << std::endl;
  std::cout << "  1: Generate C programs using LLM model" << std::endl;
  std::cout << "  2: Run sanitizer checks on generated object files" << std::endl;
  std::cout << "  3: Run differential testing on all files" << std::endl;
  std::cout << "  4: Compile all .c files in specified directory to object files" << std::endl;
  std::cout << "  5: Fix compilation errors in .c files using LLM (class-based implementation)" << std::endl;
  std::cout << "  6: Fix compilation errors in .c files using LLM (direct implementation)" << std::endl;
  std::cout << "Example: ./program 4 ./my_c_files" << std::endl;
  std::cout << "Example: ./program 6 ./buggy_c_files" << std::endl;
}

int main(int argc, char *argv[]) {
  if (argc < 2 || argc > 3) {
    displayHelp();
    return 1;
  }

  int parameter;
  try {
    parameter = std::stoi(argv[1]);
  } catch (const std::invalid_argument &e) {
    std::cout << "Invalid parameter. Please provide a valid integer (0-6)." << std::endl;
    displayHelp();
    return 1;
  }

  if (parameter < 0 || parameter > 6) {
    std::cout << "Invalid parameter. Please provide a value between 0 and 6 (inclusive)." << std::endl;
    displayHelp();
    return 1;
  }

  if (parameter == 1) {
    LLMTokensOption llmIndexedTokens;

    std::string randomCompilerOpt = llmIndexedTokens.getRandomCompilerOpt();
    std::string randomCompilerParts = llmIndexedTokens.getRandomCompilerParts();
    std::string randomPL = llmIndexedTokens.getRandomPL();
    std::string compilerFlag = llmIndexedTokens.getRandomCompilerFlag();
    std::string optLevel = llmIndexedTokens.getRandomOptLevel();
    std::string prompt =
        "Coding task: give me a program in C "
        "with all includes. No input system input is taken."
        "Please return a program (C program) and a concrete example. "
        "The C program will be with code triggering with " +
        optLevel + " and program will cover " + randomCompilerOpt +
        " optimizations part of the compiler " + randomCompilerParts +
        ", and exercises this idea in C: " + randomPL +
        ". To recap the code contains these: " + optLevel + ", " +
        randomCompilerOpt + " and " + randomCompilerParts + " and " + randomPL +
        " make sure that the program has proper symbols instead of unicodes in "
        "your response."
        " and the program MUST be a C program. ";

    std::cout << "prompt" << prompt << std::endl;
    QueryGenerator qGenerate("llama2");
    qGenerate.loadModel();
    std::string response = qGenerate.askModel(prompt);

    // Use Parser to extract and save C program
    Parser parser;
    std::string program = parser.getCProgram(response);
    std::cout << "============================" << std::endl;
    std::cout << "Program: \n" << program << std::endl;
    std::cout << "============================" << std::endl;

    auto compile_cmd = parser.getGccCommand(response);
    auto runtime_cmd = parser.getRuntimeCommand(response);

    auto [filepath, formatSuccess] = parser.parseAndSaveProgram(response);
    if (filepath.empty()) {
      std::cerr << "Error: failed writing test file\n";
      return 1;
    }

    PromptWriter promptWriter;
    auto [promptPath, promptSuccess] =
        promptWriter.savePrompt(prompt, filepath, optLevel, randomCompilerOpt,
                                randomCompilerParts, randomPL);

    if (!promptSuccess) {
      std::cerr << "Error: failed saving prompt file\n";
      return 1;
    }

    GenerateObject object;
    std::string objectPath = object.generateObjectFile(filepath, compile_cmd);
    if (objectPath.empty()) {
      std::cerr << "Error: failed generating object file\n";
      return 1;
    }
    std::cout << "Running sanitizer checks on generated object files..." << std::endl;
  } else if (parameter == 2) {
    fs::create_directories("../test");
    fs::create_directories("../object");
    fs::create_directories("../correct_code");
    fs::create_directories("../sanitizer_log");
    fs::create_directories("../sanitizer_crash");

    SanitizerProcessor sanitizer;
    bool foundFiles = false;

    try {
      for (const auto &entry : fs::directory_iterator("../object")) {
        if (entry.path().extension() == ".o") {
          foundFiles = true;
          std::cout << "Processing: " << entry.path().string() << std::endl;
          sanitizer.processSourceFile(entry.path().string());
        }
      }

      if (!foundFiles) {
        std::cout << "No object files found in ../object directory to process." << std::endl;
      }
    } catch (const fs::filesystem_error &e) {
      std::cerr << "Filesystem error: " << e.what() << std::endl;
      return 1;
    }
  } else if (parameter == 3) {
    try {
      fs::create_directories("../test");
      DifferentialTester tester;
      tester.processAllFiles();
    } catch (const fs::filesystem_error &e) {
      std::cerr << "Filesystem error: " << e.what() << std::endl;
      return 1;
    }
  } else if (parameter == 4) {
    if (argc < 3) {
      std::cout << "Please provide a directory path containing .c files" << std::endl;
      displayHelp();
      return 1;
    }
    
    std::string dirPath = argv[2];
    if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
      std::cerr << "Error: " << dirPath << " is not a valid directory" << std::endl;
      return 1;
    }
    
    compileCFilesInDirectory(dirPath);
    
    std::cout << "Files copied to ../test and compilation complete." << std::endl;
    std::cout << "You can now run sanitizer checks with option 2." << std::endl;
  } else if (parameter == 5) {
    if (argc < 3) {
      std::cout << "Please provide a directory path containing .c files with compilation errors" << std::endl;
      displayHelp();
      return 1;
    }
    
    std::string dirPath = argv[2];
    if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
      std::cerr << "Error: " << dirPath << " is not a valid directory" << std::endl;
      return 1;
    }
    
    CompilerFixer fixer;
    fixer.processDirectory(dirPath);
    
    std::cout << "Compilation error fixing complete." << std::endl;
    std::cout << "Fixed code is available in ../fixed_code directory." << std::endl;
    std::cout << "Successfully fixed files are also copied to ../test directory." << std::endl;
    std::cout << "You can now run sanitizer checks with option 2." << std::endl;
  }  else {
    std::string res = argc > 2 ? argv[2] : "";
    if (res.empty()) {
      std::cout << "Invalid parameter. Please provide a result text from LLM model." << std::endl;
      return 1;
    }
  }

  return 0;
}