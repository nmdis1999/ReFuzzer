#include "query_generator.hpp"
#include "Parser.hpp"
#include "PromptWriter.hpp"
#include "TestWriter.hpp"
#include "differential_tester.hpp"
#include "llm_tokens_options.hpp"
#include "object_generator.hpp"
#include <filesystem>
#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream> 
#include <unordered_set>

namespace fs = std::filesystem;

std::string expandUserPath(const std::string& path) {
    if (path.rfind("~/", 0) == 0) { 
        const char* home = std::getenv("HOME");
        if (home) {
            return std::string(home) + path.substr(1);
        }
    }
    return path;
}
void setupDirectories(const std::string& dirPath) {
  std::string resultDir = dirPath + "_recompiled";
  fs::create_directories(resultDir + "/correct");
  fs::create_directories(resultDir + "/incorrect");
  fs::create_directories(resultDir + "/object");
  fs::create_directories(resultDir + "/log");
  
  std::cout << "Created directories for organizing code in " << resultDir << std::endl;
}
void fixCFilesUsingRecompile(const std::string& modelName, const std::string& dirName, 
                           const std::string& compileLogDir, const std::string& sanitizeLogDir) {
  std::cout << "Running recompile to fix compilation and runtime errors using model: " << modelName << "..." << std::endl;
  
  if (compileLogDir.empty() && sanitizeLogDir.empty()) {
    std::cout << "No log directories specified, skipping recompile phase" << std::endl;
    return;
  }
  
  std::string command = "~/ReFuzzer/src/model2/recompile --dir=\"" + dirName + "\" --model=" + modelName;
  
  if (!compileLogDir.empty()) {
    command += " --compile=\"" + compileLogDir + "\"";
    std::cout << "- Will fix compilation errors from: " << compileLogDir << std::endl;
  }
  
  if (!sanitizeLogDir.empty()) {
    command += " --sanitize=\"" + sanitizeLogDir + "\"";
    std::cout << "- Will fix sanitizer errors from: " << sanitizeLogDir << std::endl;
  }
  
  command += " > recompile_output.txt 2>&1";
  
  std::cout << "Executing: " << command << std::endl;
  int result = std::system(command.c_str());
  
  std::cout << "\nRecompile output:" << std::endl;
  std::system("cat recompile_output.txt");
  
  if (result == 0) {
    std::cout << "\n✓ Recompile completed successfully." << std::endl;
  } else {
    std::cerr << "\n✗ Recompile returned with error code: " << result << std::endl;
    std::cerr << "Make sure the 'recompile' executable exists at ~/ReFuzzer/src/model2/recompile and is executable" << std::endl;
  }
}
void saveResultsToSourceDirectory(const std::string& dirPath) {
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
      if (entry.path().extension() == ".cpp") {
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
    if (entry.path().extension() == ".cpp") {
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
  
  std::string resultDir = dirPath + "_";
  fs::create_directories(resultDir + "/correct");
  fs::create_directories(resultDir + "/incorrect");
  fs::create_directories(resultDir + "/object");
  fs::create_directories(resultDir + "/log");
  
  GenerateObject objectGenerator;
  bool foundFiles = false;
  int successCount = 0;
  int failCount = 0;
  
  try {
    for (const auto& entry : fs::directory_iterator(dirPath)) {
      if (entry.is_directory()) {
        continue;
      }
      
      if (entry.path().extension() == ".cpp") {
        foundFiles = true;
        std::string filename = entry.path().filename().string();
        
        std::cout << "Processing: " << entry.path().string() << std::endl;

        std::string objectPath = objectGenerator.generateObjectFile(entry.path().string(), resultDir);
        
        if (objectPath.empty()) {
          fs::path incorrectPath = fs::path(resultDir + "/incorrect") / filename;
          fs::copy_file(entry.path(), incorrectPath, fs::copy_options::overwrite_existing);
          std::cout << "Compilation failed - copied to: " << incorrectPath.string() << std::endl;
          failCount++;
        } else {
          fs::path correctPath = fs::path(resultDir + "/correct") / filename;
          fs::copy_file(entry.path(), correctPath, fs::copy_options::overwrite_existing);
          std::cout << "Compilation successful - copied to: " << correctPath.string() << std::endl;
          std::cout << "Object file created: " << objectPath << std::endl;
          successCount++;
        }
      }
    }
    
    if (!foundFiles) {
      std::cout << "No .cpp files found in " << dirPath << " directory to process." << std::endl;
    } else {
      std::cout << "\nProcessed " << (successCount + failCount) << " files: " 
                << successCount << " successful, " << failCount << " failed" << std::endl;
    }
    
  } catch (const fs::filesystem_error& e) {
    std::cerr << "Filesystem error: " << e.what() << std::endl;
  }
}

void displayHelp() {
  std::cout << "Usage: ./program <command> [options] [directory_path]" << std::endl;
  std::cout << "Commands:" << std::endl;
  std::cout << "  generate      Generate C++ programs using LLM model" << std::endl;
  std::cout << "  sanitize      Run sanitizer checks on generated object files" << std::endl;
  std::cout << "  compile       Process and compile all .c files in specified directory" << std::endl;
  std::cout << "  refuzz        Fix compilation errors, run sanitizers, and organize" << std::endl;
  std::cout << "                files into correct/incorrect subdirectories" << std::endl;
  std::cout << "  help          Display this help message" << std::endl;
  std::cout << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  --model=<name>  Specify the Ollama model to use (default: llama3.2)" << std::endl;
  std::cout << "                  Example: --model=llama3" << std::endl;
  std::cout << "  --clang=<path>  Path to AFL-instrumented Clang (default: /usr/local/llvm/bin/clang)" << std::endl;
  std::cout << "  --gcc=<path>    Path to AFL-instrumented GCC (default: afl-gcc)" << std::endl;
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

std::string parseOption(int argc, char *argv[], const std::string& option, const std::string& defaultValue) {
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg.find(option) == 0) {
      return arg.substr(option.length());
    }
  }
  
  return defaultValue;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    displayHelp();
    return 1;
  }
  std::string command = argv[1];

  if (command == "generate" || command == "gen") {
    std::string modelName = parseModelOption(argc, argv);
    std::string dirName = parseOption(argc, argv, "--dir=", "../test");
    dirName = expandUserPath(dirName);
    LLMTokensOption llmIndexedTokens;

    std::string randomCompilerOpt = llmIndexedTokens.getRandomCompilerOpt();
    std::string randomCompilerParts = llmIndexedTokens.getRandomCompilerParts();
    std::string randomPL = llmIndexedTokens.getRandomPL();
    std::string compilerFlag = llmIndexedTokens.getRandomCompilerFlag();
    std::string optLevel = llmIndexedTokens.getRandomOptLevel();
    
    std::string prompt =
    "Coding task: write a C++ program that is fully self-contained and does not rely on any user input, file reading/writing, or external libraries beyond standard headers. "
    "It should include only standard headers, use hardcoded values, and demonstrate a concept such as sorting, simulation, or arithmetic. "
    "Do not use std::cin, file I/O, or any dynamic input. Output a valid, complete C++ program with main() and all includes. No markdown or Unicode formatting — only plain C++ code. "
    "Coding task: give me a program in C++ "
    "with all includes. No input system input is taken. "
    "Please return a program (C++ program) and a concrete example. "
    "The C++ program will be with code triggering with "
    " and program will trigger" + randomCompilerOpt +
    " optimizations part of the compiler " + randomCompilerParts +
    ", and exercises this idea in C++: " + randomPL +
    ". To recap the code contains these: " +
    randomCompilerOpt + " and " + randomCompilerParts + " and " + randomPL +
    " make sure that the program has proper symbols instead of unicodes in "
    "your response."
    " and the program MUST be a C++ program. ";

    QueryGenerator qGenerate(modelName);
    qGenerate.loadModel();
    std::string response = qGenerate.askModel(prompt);

    std::cout << "=== RAW RESPONSE ===" << std::endl;
    std::cout << response << std::endl;
    std::cout << "===================" << std::endl;
    
    auto [filepath, formatSuccess] = Parser::parseAndSaveProgram(response, "", dirName);
    if (filepath.empty()) {
      std::cerr << "Error: failed writing test file\n";
      return 1;
    }

    std::string promptDir;
    if (fs::exists(dirName) && fs::is_directory(dirName)) {
        promptDir = (fs::path(dirName) / "prompt").string();
    } else {
        promptDir = (fs::path("../test") / "prompt").string();
    }
     PromptWriter promptWriter(promptDir);
    auto [promptPath, promptSuccess] =
        promptWriter.savePrompt(prompt, filepath, optLevel, randomCompilerOpt,
                                randomCompilerParts, randomPL);

    if (!promptSuccess) {
      std::cerr << "Error: failed saving prompt file\n";
      return 1;
    }

    GenerateObject object;
    std::string objectPath = object.generateObjectFile(filepath, dirName);
    if (objectPath.empty()) {
      std::cerr << "Error: failed generating object file\n";
      return 1;
    }
    std::cout << "Please run sanitizer checks on generated object files... use \"sanitize (san)\" option" << std::endl;
  } else if (command == "compile") {
    if (argc < 3) {
      std::cout << "Please provide a directory path containing .c files" << std::endl;
      displayHelp();
      return 1;
    }
    //TODO: change this to --dir-=option later
    std::string dirPath = argv[2];
    dirPath = expandUserPath(dirPath);
    if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
      std::cerr << "Error: " << dirPath << " is not a valid directory" << std::endl;
      return 1;
    }
    
    compileCFilesInDirectory(dirPath);
    
    std::cout << "Files copied to ../test and compilation complete." << std::endl;
    std::cout << "You can now run sanitizer checks with given option in help." << std::endl;
  }  else if (command == "sanitize" || command == "san") {
    // Always mention to the user that if directory is not provided the code 
    // will look for ../test directory
    std::string dirName = parseOption(argc, argv, "--dir=", "../test");
    dirName = expandUserPath(dirName);
    if (!fs::exists(dirName) || !fs::is_directory(dirName)) {
      std::cerr << "Error: " << dirName << " is not a valid directory" << std::endl;
      return 1;
    }
  
    std::string sanLog = dirName + "/sanitizer_log";
    std::string correctDir = dirName + "/correct";
    std::string incorrectDir = dirName + "/incorrect";
    std::string objectDir = dirName + "/object";
    
    fs::create_directories(sanLog);
    fs::create_directories(correctDir);
    fs::create_directories(incorrectDir);
    fs::create_directories(objectDir);
    
    std::cout << "Running sanitizers on C++ files in: " << dirName << std::endl;
    std::cout << "If directory is not provided, default ../test will be used" << std::endl;
    std::cout << "Results will be organized inside: " << dirName << std::endl;
    
    int totalFiles = 0;
    int correctFiles = 0;
    int incorrectFiles = 0;
    
    try {
      for (const auto& entry : fs::directory_iterator(dirName)) {
        if (entry.path().extension() == ".cpp") {
          totalFiles++;
          std::string filename = entry.path().filename().string();
          std::string basename = entry.path().stem().string();
          std::string filepath = entry.path().string();
          
          std::cout << "\nProcessing: " << filename << std::endl;
          
          bool hasErrors = false;
          std::vector<std::string> sanitizers = {"asan", "msan", "ubsan"};
          std::vector<std::string> createdBinaries;
          std::string logFile = sanLog + "/" + basename + ".log"; 
          
          for (const std::string& sanitizer : sanitizers) {
            std::cout << "  Running " << sanitizer << "..." << std::endl;
            
            std::string binaryName = "./" + basename + "_" + sanitizer;
            std::string compileCmd, runCmd;
            
            if (sanitizer == "asan") {
              compileCmd = "clang++ -fsanitize=address -O0 -w -fno-omit-frame-pointer -g \"" + 
                          filepath + "\" -o \"" + binaryName + "\" 2>/dev/null";
              runCmd = "timeout 30s bash -c \"ASAN_OPTIONS=detect_stack_use_after_return=1 " + 
                      binaryName + "\" 2>&1";
            } else if (sanitizer == "msan") {
              compileCmd = "clang++ -fsanitize=memory -fno-omit-frame-pointer -g -O0 -w \"" + 
                          filepath + "\" -o \"" + binaryName + "\" 2>/dev/null";
              runCmd = "timeout 30s " + binaryName + " 2>&1";
            } else if (sanitizer == "ubsan") {
              compileCmd = "clang++ -fsanitize=undefined -g -O1 -w \"" + 
                          filepath + "\" -o \"" + binaryName + "\" 2>/dev/null";
              runCmd = "timeout 30s bash -c \"UBSAN_OPTIONS=abort_on_error=1:print_stacktrace=1 " + 
                      binaryName + "\" 2>&1";
            }
            
            int compileResult = std::system(compileCmd.c_str());
            if (compileResult != 0) {
              std::cout << "    " << sanitizer << " compilation failed" << std::endl;
              hasErrors = true;
              // TODO: Add compilation error logging to the main log file
              continue;
            }
            
            createdBinaries.push_back(binaryName);
            
            std::string captureCmd = "(" + runCmd + ") > /tmp/" + basename + "_" + sanitizer + "_output.txt 2>&1";
            int runResult = std::system(captureCmd.c_str());
            
            if (runResult == 0) {
              std::cout << "    " << sanitizer << " - OK" << std::endl;
              std::system(("rm -f /tmp/" + basename + "_" + sanitizer + "_output.txt").c_str());
            } else if (runResult == 124 * 256) { 
              std::cout << "    " << sanitizer << " - TIMEOUT" << std::endl;
              hasErrors = true;

              std::string appendCmd = "echo \"=== " + sanitizer + " START ===\" >> \"" + logFile + "\" && " +
                                    "cat /tmp/" + basename + "_" + sanitizer + "_output.txt >> \"" + logFile + "\" && " +
                                    "echo \"=== " + sanitizer + " END ===\" >> \"" + logFile + "\"";
              std::system(appendCmd.c_str());
              std::system(("rm -f /tmp/" + basename + "_" + sanitizer + "_output.txt").c_str());
            } else {
              std::cout << "    " << sanitizer << " - ERROR DETECTED (exit code: " << (runResult / 256) << ")" << std::endl;
              hasErrors = true;

              std::string appendCmd = "echo \"=== " + sanitizer + " START ===\" >> \"" + logFile + "\" && " +
                                    "cat /tmp/" + basename + "_" + sanitizer + "_output.txt >> \"" + logFile + "\" && " +
                                    "echo \"=== " + sanitizer + " END ===\" >> \"" + logFile + "\"";
              std::system(appendCmd.c_str());
              std::system(("rm -f /tmp/" + basename + "_" + sanitizer + "_output.txt").c_str());
            }
          }
          if (!hasErrors) {
            std::string cleanExecutable = objectDir + "/" + basename;
            std::string createExecCmd = "clang++ -O2 \"" + filepath + "\" -o \"" + cleanExecutable + "\" 2>/dev/null";
            
            int execResult = std::system(createExecCmd.c_str());
            if (execResult == 0) {
              std::cout << "  Clean executable created: " << cleanExecutable << std::endl;
            } else {
              std::cout << "  Warning: Failed to create clean executable" << std::endl;
            }
            fs::path correctPath = fs::path(correctDir) / filename;
            try {
              fs::copy_file(filepath, correctPath, fs::copy_options::overwrite_existing);
              correctFiles++;
              std::cout << "  Result: NO ISSUES - moved to correct/, executable in object/" << std::endl;
            } catch (const fs::filesystem_error& e) {
              std::cerr << "  Error copying file to correct/: " << e.what() << std::endl;
            }
          } else {
            fs::path incorrectPath = fs::path(incorrectDir) / filename;
            try {
              fs::copy_file(filepath, incorrectPath, fs::copy_options::overwrite_existing);
              incorrectFiles++;
              std::cout << "  Result: ERRORS DETECTED - moved to incorrect/" << std::endl;
            } catch (const fs::filesystem_error& e) {
              std::cerr << "  Error copying file to incorrect/: " << e.what() << std::endl;
            }
          }
          for (const std::string& binary : createdBinaries) {
            std::system(("rm -f \"" + binary + "\"").c_str());
          }
        }
      }
      
      if (totalFiles == 0) {
        std::cout << "No .cpp files found in " << dirName << " directory." << std::endl;
      } else {
        std::cout << "\n=== SANITIZER SUMMARY ===" << std::endl;
        std::cout << "Total files processed: " << totalFiles << std::endl;
        std::cout << "Files with no issues: " << correctFiles << std::endl;
        std::cout << "Files with errors detected: " << incorrectFiles << std::endl;
        std::cout << "\nResults organized in:" << std::endl;
        std::cout << "  " << correctDir << "/ - Clean source files" << std::endl;
        std::cout << "  " << objectDir << "/ - Clean executables" << std::endl;
        std::cout << "  " << incorrectDir << "/ - Files with detected errors" << std::endl;
        std::cout << "  " << sanLog << "/ - Error logs" << std::endl;
      }
      
    } catch (const fs::filesystem_error& e) {
      std::cerr << "Filesystem error: " << e.what() << std::endl;
      return 1;
    }
} else if (command == "refuzz" || command == "rf") {
    std::string modelName = parseModelOption(argc, argv);
    std::string dirName = parseOption(argc, argv, "--dir=", "../test");
    std::string compileLogDir = parseOption(argc, argv, "--compile=", "");
    std::string sanitizeLogDir = parseOption(argc, argv, "--sanitize=", "");
    
    // Expand user paths
    dirName = expandUserPath(dirName);
    if (!compileLogDir.empty()) compileLogDir = expandUserPath(compileLogDir);
    if (!sanitizeLogDir.empty()) sanitizeLogDir = expandUserPath(sanitizeLogDir);
    
    if (!fs::exists(dirName) || !fs::is_directory(dirName)) {
      std::cerr << "Error: " << dirName << " is not a valid directory" << std::endl;
      return 1;
    }
    
    // Validate log directories if provided
    if (!compileLogDir.empty() && (!fs::exists(compileLogDir) || !fs::is_directory(compileLogDir))) {
      std::cerr << "Error: Compilation log directory " << compileLogDir << " does not exist" << std::endl;
      return 1;
    }
    
    if (!sanitizeLogDir.empty() && (!fs::exists(sanitizeLogDir) || !fs::is_directory(sanitizeLogDir))) {
      std::cerr << "Error: Sanitizer log directory " << sanitizeLogDir << " does not exist" << std::endl;
      return 1;
    }
    
    std::cout << "=== REFUZZ MODE ===" << std::endl;
    std::cout << "Using model: " << modelName << " for refuzzing..." << std::endl;
    std::cout << "Target directory: " << dirName << std::endl;
    if (!compileLogDir.empty()) std::cout << "Compilation logs: " << compileLogDir << std::endl;
    if (!sanitizeLogDir.empty()) std::cout << "Sanitizer logs: " << sanitizeLogDir << std::endl;
    
    setupDirectories(dirName);

    if (!compileLogDir.empty() || !sanitizeLogDir.empty()) {
      std::cout << "\n=== PHASE 1: FIXING ERRORS ===" << std::endl;
      fixCFilesUsingRecompile(modelName, dirName, compileLogDir, sanitizeLogDir);
    } else {
      std::cout << "\n=== PHASE 1: SKIPPING RECOMPILE ===" << std::endl;
      std::cout << "No log directories specified with --compile or --sanitize" << std::endl;
    }
    
    std::cout << "\n=== PHASE 2: SANITIZER CHECKS ===" << std::endl;
    std::cout << "Running sanitizer checks on all files..." << std::endl;
    std::string sanitizeCmd = "./query_generator sanitize --dir=\"" + dirName + "\"";
    std::cout << "Executing: " << sanitizeCmd << std::endl;
    int sanitizeResult = std::system(sanitizeCmd.c_str());
    
    if (sanitizeResult == 0) {
      std::cout << "✓ Sanitizer checks completed" << std::endl;
    } else {
      std::cerr << "✗ Sanitizer checks failed" << std::endl;
    }

    std::cout << "\n=== PHASE 3: ORGANIZING RESULTS ===" << std::endl;
    saveResultsToSourceDirectory(dirName);
    
    std::string resultDir = dirName + "_recompiled";
    std::cout << "\n=== REFUZZ COMPLETE ===" << std::endl;
    std::cout << "Results organized in: " << resultDir << std::endl;
    std::cout << "✓ Correct files: " << resultDir << "/correct" << std::endl;
    std::cout << "✓ Incorrect files: " << resultDir << "/incorrect" << std::endl;
    std::cout << "✓ Object files: " << resultDir << "/object" << std::endl;
  }
}