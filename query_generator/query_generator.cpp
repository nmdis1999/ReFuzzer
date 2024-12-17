#include <iostream>
#include <string>
#include <filesystem>
#include "Parser.hpp"
#include "PromptWriter.hpp"
#include "TestWriter.hpp"
#include "llm_tokens_options.hpp"
#include "object_generator.hpp"
#include "query_generator.hpp"
#include "sanitizer_processor.hpp"
#include "differential_tester.hpp"

namespace fs = std::filesystem;

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 2) {
        std::cout << "Please provide single parameter (0, 1, 2, 3)\n";
        return 1;
    }

    int parameter;
    try {
        parameter = std::stoi(argv[1]);
    } catch (const std::invalid_argument &e) {
        std::cout << "Invalid parameter. Please provide a valid integer (0, 1, 2, or 3)." << std::endl;
        return 1;
    }

    if (parameter < 0 || parameter > 3) {
        std::cout << "Invalid parameter. Please provide a value between 0 and 3 (inclusive)." << std::endl;
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
            "with all includes. No input is "
            "taken."
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
            for (const auto& entry : fs::directory_iterator("../object")) {
                if (entry.path().extension() == ".o") {
                    foundFiles = true;
                    std::cout << "Processing: " << entry.path().string() << std::endl;
                    sanitizer.processSourceFile(entry.path().string());
                }
            }

            if (!foundFiles) {
                std::cout << "No object files found in ../object directory to process." << std::endl;
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Filesystem error: " << e.what() << std::endl;
            return 1;
        }
    } else if (parameter == 3) {
        try {
            fs::create_directories("../test");
            DifferentialTester tester;
            tester.processAllFiles();
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Filesystem error: " << e.what() << std::endl;
            return 1;
        }
    } else {
        std::string res = argc > 2 ? argv[2] : "";
        if (res.empty()) {
            std::cout << "Invalid parameter. Please provide a result text from LLM model." << std::endl;
            return 1;
        }
    }

    return 0;
}
