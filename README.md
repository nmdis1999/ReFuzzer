# Compiler Fuzzing Seed Generator

A tool that leverages Large Language Models (LLM) to generate seeds for compiler fuzzing.

## Prerequisites

The following tools and dependencies are required to build and run the project:

- Python 3.13.1 or later
- CMake 3.x
- Clang 20
- GCC 13
- Ollama
- Conan 2.x (package manager)

## Building the Project

1. Clone the repository:

```bash
git clone https://github.com/nmdis1999/SmartCompilerTesting.git
cd SmartCompilerTesting
```

2. Set up Conan and install dependencies:

```bash
conan profile detect
conan install . --build=missing
```

3. Configure the project with CMake:

```bash
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=build/generators/conan_toolchain.cmake
```

4. Build the project:

```bash
cmake --build build
```

5. Using Docker

```

```
