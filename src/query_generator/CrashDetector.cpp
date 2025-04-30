#include "CrashDetector.hpp"

CrashDetector::CrashDetector(const std::string& clangPath, const std::string& gccPath) 
    : m_clangPath(clangPath),
      m_gccPath(gccPath),
      m_crashesDir("../crashes"),
      m_crashLogPath("../crashes/crash_log.txt"),
      m_totalFilesTested(0),
      m_clangCrashes(0),
      m_gccCrashes(0) {
    
    // Create crashes directory and subdirectories
    fs::create_directories(m_crashesDir + "/clang");
    fs::create_directories(m_crashesDir + "/gcc");
    
    std::cout << "CrashDetector initialized with:" << std::endl;
    std::cout << "  Clang path: " << m_clangPath << std::endl;
    std::cout << "  GCC path: " << m_gccPath << std::endl;
    std::cout << "  Crashes directory: " << m_crashesDir << std::endl;
}

std::string CrashDetector::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string CrashDetector::getCompilerFlags(const std::string& filePath) const {
    // Basic flags for syntax-only checking
    std::string flags = "-fsyntax-only";
    
    // Add language-specific flags based on file extension
    std::string ext = fs::path(filePath).extension().string();
    if (ext == ".cpp" || ext == ".cc" || ext == ".cxx") {
        flags += " -std=c++17";
    } else if (ext == ".c") {
        flags += " -std=c11";
    }
    
    return flags;
}

CrashDetector::CompilerResult 
CrashDetector::testCompiler(const std::string& compiler, const std::string& filePath) {
    CompilerResult result;
    
    // Create a temporary file to capture the output
    std::string tempOutput = "/tmp/compiler_output.txt";
    
    // Prepare the command - use the exact same approach as in test-compiler
    std::string flags = getCompilerFlags(filePath);
    
    // Construct the command the same way as in test-compiler
    std::string command = compiler + " " + flags + " " + filePath;
    
    // Log the command for debugging
    std::cout << "  Running: " << command << std::endl;
    
    // Execute the command - same as in test-compiler
    result.exitCode = system(command.c_str());
    result.exitCode = WEXITSTATUS(result.exitCode);
    
    // Read the result from stderr/stdout
    // For simplicity, we'll just set a basic message
    if (result.exitCode == 0) {
        result.output = "Compiler ran successfully";
    } else if (result.exitCode == 127) {
        result.output = "Command not found: " + compiler;
    } else {
        result.output = "Compiler failed with exit code: " + std::to_string(result.exitCode);
    }
    
    return result;
}

void CrashDetector::processCrash(const std::string& compiler, 
                                const std::string& filePath,
                                int exitCode, 
                                const std::string& output) {
    std::string filename = fs::path(filePath).filename().string();
    std::string timestamp = getCurrentTimestamp();
    
    // Create crash directory for this compiler if it doesn't exist
    std::string compilerDir = m_crashesDir + "/" + compiler;
    fs::create_directories(compilerDir);
    
    // Copy the crashing file
    std::string destPath = compilerDir + "/crash_" + std::to_string(exitCode) + "_" + filename;
    try {
        fs::copy_file(filePath, destPath, fs::copy_options::overwrite_existing);
        std::cout << "Copied crashing file to: " << destPath << std::endl;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error copying file: " << e.what() << std::endl;
    }
    
    // Save the crash output to a log file
    std::string errorPath = destPath + ".error";
    std::ofstream errorFile(errorPath);
    if (errorFile) {
        errorFile << "Exit Code: " << exitCode << std::endl;
        errorFile << "Timestamp: " << timestamp << std::endl;
        errorFile << "Compiler: " << compiler << std::endl;
        errorFile << "Output:" << std::endl;
        errorFile << output << std::endl;
        errorFile.close();
    }
    
    // Append to the main crash log
    std::ofstream logFile(m_crashLogPath, std::ios::app);
    if (logFile) {
        logFile << "=================================" << std::endl;
        logFile << "Crash detected at " << timestamp << std::endl;
        logFile << "File: " << filename << std::endl;
        logFile << "Compiler: " << compiler << std::endl;
        logFile << "Exit Code: " << exitCode << std::endl;
        logFile << "Output:" << std::endl;
        logFile << output << std::endl;
        logFile << "=================================" << std::endl << std::endl;
        logFile.close();
    }
    
    // Update crash counter
    if (compiler == "clang") {
        m_clangCrashes++;
    } else if (compiler == "gcc") {
        m_gccCrashes++;
    }
}

void CrashDetector::detectCrashesInDirectory(const std::string& dirPath) {
    // Reset counters
    m_totalFilesTested = 0;
    m_clangCrashes = 0;
    m_gccCrashes = 0;
    
    std::cout << "Running crash detection on files in: " << dirPath << std::endl;
    
    // Initialize or append to the crash log
    std::ofstream logFile(m_crashLogPath, std::ios::app);
    if (logFile) {
        logFile << "===== Crash Detection Run: " << getCurrentTimestamp() << " =====" << std::endl;
        logFile << "Directory: " << dirPath << std::endl << std::endl;
        logFile.close();
    }
    
    try {
        // Process all C and C++ files in the directory
        for (const auto& entry : fs::directory_iterator(dirPath)) {
            std::string ext = entry.path().extension().string();
            if (ext == ".c" || ext == ".cpp" || ext == ".cc" || ext == ".cxx") {
                std::string filePath = entry.path().string();
                std::string filename = entry.path().filename().string();
                
                m_totalFilesTested++;
                std::cout << "Testing: " << filename << std::endl;
                
                // Test with Clang
                std::cout << "  Testing with Clang... ";
                auto clangResult = testCompiler(m_clangPath, filePath);
                if (clangResult.isCrash()) {
                    std::cout << "CRASH detected! (Exit code: " << clangResult.exitCode << ")" << std::endl;
                    processCrash("clang", filePath, clangResult.exitCode, clangResult.output);
                } else {
                    std::cout << "No crash" << std::endl;
                }
                
                // Test with GCC
                std::cout << "  Testing with GCC... ";
                auto gccResult = testCompiler(m_gccPath, filePath);
                if (gccResult.isCrash()) {
                    std::cout << "CRASH detected! (Exit code: " << gccResult.exitCode << ")" << std::endl;
                    processCrash("gcc", filePath, gccResult.exitCode, gccResult.output);
                } else {
                    std::cout << "No crash" << std::endl;
                }
                
                std::cout << "---------------------" << std::endl;
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
    
    // Add summary to the log
    std::ofstream summaryFile(m_crashLogPath, std::ios::app);
    if (summaryFile) {
        summaryFile << "Summary:" << std::endl;
        summaryFile << "  Files tested: " << m_totalFilesTested << std::endl;
        summaryFile << "  Clang crashes: " << m_clangCrashes << std::endl;
        summaryFile << "  GCC crashes: " << m_gccCrashes << std::endl;
        summaryFile << "  Total crashes: " << (m_clangCrashes + m_gccCrashes) << std::endl;
        summaryFile << "=====================================" << std::endl << std::endl;
        summaryFile.close();
    }
    
    // Output summary to console
    std::cout << std::endl;
    std::cout << "Crash detection completed:" << std::endl;
    std::cout << "  Files tested: " << m_totalFilesTested << std::endl;
    std::cout << "  Clang crashes: " << m_clangCrashes << std::endl;
    std::cout << "  GCC crashes: " << m_gccCrashes << std::endl;
    std::cout << "  Total crashes: " << (m_clangCrashes + m_gccCrashes) << std::endl;
    std::cout << "  Crash reports saved to: " << m_crashesDir << std::endl;
    std::cout << "  Detailed log: " << m_crashLogPath << std::endl;
}