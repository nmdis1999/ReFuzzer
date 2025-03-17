#include "query_generator.hpp"
#include "Parser.hpp"
#include "PromptWriter.hpp"
#include "TestWriter.hpp"
#include "differential_tester.hpp"
#include "llm_tokens_options.hpp"
#include "object_generator.hpp"
#include "sanitizer_processor.hpp"
#include <filesystem>
#include <iostream>
#include <string>
#include <cstdlib>

namespace fs = std::filesystem;

void setupDirectories(const std::string& dirPath) {
  namespace fs = std::filesystem;
  
  fs::create_directories("../test");
  fs::create_directories("../object");
  fs::create_directories("../log");
  fs::create_directories("../sanitizer_log");
  fs::create_directories("../sanitizer_crash");
  
  std::string resultDir = dirPath + "_recompiled";
  fs::create_directories(resultDir + "/correct");
  fs::create_directories(resultDir + "/incorrect");
  
  std::cout << "Created directories for organizing code in " << resultDir << std::endl;
}

void saveResultsToSourceDirectory(const std::string& dirPath) {
  namespace fs = std::filesystem;
  
  std::string resultDir = dirPath + "_recompiled";
  std::cout << "Organizing results in " << resultDir << "..." << std::endl;
  
  if (fs::exists(resultDir + "/correct")) {
    for (const auto& entry : fs::directory_iterator(resultDir + "/correct")) {
      fs::remove(entry.path());
    }
  }
  
  if (fs::exists(resultDir + "/incorrect")) {
    for (const auto& entry : fs::directory_iterator(resultDir + "/incorrect")) {
      fs::remove(entry.path());
    }
  }
  
  std::unordered_set<std::string> processedFiles;
  
  if (fs::exists("../correct_code")) {
    for (const auto& entry : fs::directory_iterator("../correct_code")) {
      if (entry.path().extension() == ".c") {
        std::string filename = entry.path().filename().string();
        fs::path destPath = fs::path(resultDir) / "correct" / filename;
        
        if (processedFiles.find(filename) == processedFiles.end()) {
          try {
            fs::copy_file(entry.path(), destPath, fs::copy_options::overwrite_existing);
            std::cout << "Copied correct file to: " << destPath.string() << std::endl;
            processedFiles.insert(filename);
          } catch (const fs::filesystem_error& e) {
            std::cerr << "Error copying file: " << e.what() << std::endl;
          }
        }
      }
    }
  }
  
  for (const auto& entry : fs::directory_iterator("../test")) {
    if (entry.path().extension() == ".c") {
      std::string filename = entry.path().filename().string();
      std::string basename = entry.path().stem().string();
      fs::path objectPath = "../object/" + basename + ".o";
      
      if (processedFiles.find(filename) != processedFiles.end()) {
        continue;
      }
      
      if (!fs::exists(objectPath)) {
        fs::path destPath = fs::path(resultDir) / "incorrect" / filename;
        try {
          fs::copy_file(entry.path(), destPath, fs::copy_options::overwrite_existing);
          std::cout << "Copied incorrect file to: " << destPath.string() << std::endl;
          processedFiles.insert(filename);
        } catch (const fs::filesystem_error& e) {
          std::cerr << "Error copying file: " << e.what() << std::endl;
        }
      } else {
        fs::path correctPath = "../correct_code/" + filename;
        if (!fs::exists(correctPath)) {
          fs::path destPath = fs::path(resultDir) / "incorrect" / filename;
          try {
            fs::copy_file(entry.path(), destPath, fs::copy_options::overwrite_existing);
            std::cout << "Copied file with object but with possible sanitizer errors to: " << destPath.string() << std::endl;
            processedFiles.insert(filename);
          } catch (const fs::filesystem_error& e) {
            std::cerr << "Error copying file: " << e.what() << std::endl;
          }
        }
      }
    }
  }
  
  int correctCount = 0;
  int incorrectCount = 0;
  
  if (fs::exists(resultDir + "/correct")) {
    for (const auto& entry : fs::directory_iterator(resultDir + "/correct")) {
      correctCount++;
    }
  }
  
  if (fs::exists(resultDir + "/incorrect")) {
    for (const auto& entry : fs::directory_iterator(resultDir + "/incorrect")) {
      incorrectCount++;
    }
  }
  
  std::cout << "Finished organizing files in " << resultDir << std::endl;
  std::cout << "Summary: " << correctCount << " correct files, " 
            << incorrectCount << " incorrect files" << std::endl;
  std::cout << "Total: " << (correctCount + incorrectCount) << " files processed" << std::endl;
}

void compileCFilesInDirectory(const std::string& dirPath) {
  namespace fs = std::filesystem;
  
  setupDirectories(dirPath);
  
  if (fs::exists("../test")) {
    fs::remove_all("../test");
  }
  fs::create_directories("../test");
  
  GenerateObject objectGenerator;
  
  bool foundFiles = false;
  try {
    for (const auto& entry : fs::directory_iterator(dirPath)) {
      if (entry.is_directory() && 
          (entry.path().filename() == "correct" || entry.path().filename() == "incorrect")) {
        continue;
      }
      
      if (entry.path().extension() == ".c") {
        foundFiles = true;
        std::cout << "Processing: " << entry.path().string() << std::endl;
        
        std::string filename = entry.path().filename().string();
        fs::path destPath = "../test/" + filename;
        fs::copy_file(entry.path(), destPath, fs::copy_options::overwrite_existing);
        std::cout << "Copied to: " << destPath.string() << std::endl;
        
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

void fixCFilesUsingRecompile(const std::string& modelName) {
  std::cout << "Running recompile to fix compilation and runtime errors using model: " << modelName << "..." << std::endl;
  
  std::string command = "cd ../model2 && ./recompile --model=" + modelName + " > ../recompile_output.txt 2>&1";
  int result = system(command.c_str());
  
  std::cout << "Recompile output (from recompile_output.txt):" << std::endl;
  system("cat ../recompile_output.txt");
  
  if (result == 0) {
    std::cout << "Recompile completed successfully." << std::endl;
  } else {
    std::cerr << "Recompile returned with error code: " << result << std::endl;
  }
}


void displayHelp() {
  std::cout << "Usage: ./program <command> [options] [directory_path]" << std::endl;
  std::cout << "Commands:" << std::endl;
  std::cout << "  generate      Generate C programs using LLM model" << std::endl;
  std::cout << "  sanitize      Run sanitizer checks on generated object files" << std::endl;
  std::cout << "  difftest      Run differential testing on all files" << std::endl;
  std::cout << "  compile       Process and compile all .c files in specified directory" << std::endl;
  std::cout << "  refuzz        Fix compilation errors, run sanitizers, and organize" << std::endl;
  std::cout << "                files into correct/incorrect subdirectories" << std::endl;
  std::cout << "  help          Display this help message" << std::endl;
  std::cout << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  --model=<name>  Specify the Ollama model to use (default: llama2)" << std::endl;
  std::cout << "                  Example: --model=llama3" << std::endl;
}

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

int main(int argc, char *argv[]) {
  if (argc < 2) {
    displayHelp();
    return 1;
  }
  std::string command = argv[1];

  if (command == "generate" || command == "gen") {
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

    std::string program = Parser::getCProgram(response);
    std::cout << "============================" << std::endl;
    std::cout << "Program: \n" << program << std::endl;
    std::cout << "============================" << std::endl;

    auto compile_cmd = Parser::getGccCommand(response);
    auto runtime_cmd = Parser::getRuntimeCommand(response);

    auto [filepath, formatSuccess] = Parser::parseAndSaveProgram(response);
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
  } else if (command == "sanitize" || command == "san") {
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
  } else if (command == "difftest") {
    try {
      fs::create_directories("../test");
      DifferentialTester tester;
      tester.processAllFiles();
    } catch (const fs::filesystem_error &e) {
      std::cerr << "Filesystem error: " << e.what() << std::endl;
      return 1;
    }
  } else if (command == "compile") {
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
  } else if (command == "refuzz" || command == "rf") {
    std::string modelName = parseModelOption(argc, argv);
    
    std::string dirPath;
    for (int i = 2; i < argc; i++) {
      std::string arg = argv[i];
      if (arg.find("--") != 0) { 
        dirPath = arg;
        break;
      }
    }
    
    if (dirPath.empty()) {
      std::cout << "Please provide the source directory path" << std::endl;
      displayHelp();
      return 1;
    }
    
    if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
      std::cerr << "Error: " << dirPath << " is not a valid directory" << std::endl;
      return 1;
    }
    
    std::cout << "Using model: " << modelName << " for refuzzing..." << std::endl;
    
    setupDirectories(dirPath);
    
    fixCFilesUsingRecompile(modelName);
    
    SanitizerProcessor sanitizer;
    try {
      for (const auto &entry : fs::directory_iterator("../object")) {
        if (entry.path().extension() == ".o") {
          std::cout << "Processing: " << entry.path().string() << std::endl;
          sanitizer.processSourceFile(entry.path().string());
        }
      }
    } catch (const fs::filesystem_error &e) {
      std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
    
    saveResultsToSourceDirectory(dirPath);

    std::string resultDir = dirPath + "_recompiled";
    std::cout << "Recompilation and organization complete." << std::endl;
    std::cout << "Correct files are in: " << resultDir << "/correct" << std::endl;
    std::cout << "Incorrect files are in: " << resultDir << "/incorrect" << std::endl;
  }  else if (command == "help" || command == "h" || command == "--help" || command == "-h") {
    displayHelp(); 
  } else {
    std::cout << "Unknown command: " << command << std::endl;
    displayHelp();
    return 1;
  }
  return 0;
}