#ifndef CRASH_DETECTOR_HPP
#define CRASH_DETECTOR_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <chrono>
#include <ctime>

namespace fs = std::filesystem;

class CrashDetector {
public:
    struct CompilerResult {
        int exitCode;
        std::string output;
        bool isCrash() const { return exitCode != 0 && exitCode != 1; }
    };

    struct CrashInfo {
        std::string compiler;
        std::string filename;
        int exitCode;
        std::string output;
        std::string timestamp;
    };

    CrashDetector(const std::string& clangPath = "/usr/local/llvm/bin/clang", 
                  const std::string& gccPath = "afl-gcc");

    // Run crash detection on all files in a directory
    void detectCrashesInDirectory(const std::string& dirPath);

    // Get summary info
    int getTotalFilesTested() const { return m_totalFilesTested; }
    int getClangCrashes() const { return m_clangCrashes; }
    int getGccCrashes() const { return m_gccCrashes; }

private:
    std::string m_clangPath;
    std::string m_gccPath;
    std::string m_crashesDir;
    std::string m_crashLogPath;
    int m_totalFilesTested;
    int m_clangCrashes;
    int m_gccCrashes;

    // Helper methods
    std::string getCurrentTimestamp() const;
    CompilerResult testCompiler(const std::string& compiler, const std::string& filePath);
    void processCrash(const std::string& compiler, const std::string& filePath, 
                      int exitCode, const std::string& output);
    std::string getCompilerFlags(const std::string& filePath) const;
};

#endif // CRASH_DETECTOR_HPP