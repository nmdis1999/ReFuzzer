# Compiler Fuzzing Seed Generator

A tool that leverages open-source Large Language Models (LLMs) to generate and repair seeds for compiler fuzzing.

## Features

- Blackbox fuzzer to generate C programs using Ollama model
- ReFuzzer component to repair and validate generated code by fixing compilation and runtime errors

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

```base
cmake . -DCMAKE_TOOLCHAIN_FILE=build/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
```

4. Build the project:

```bash
cmake --build build
```

5. Using Docker

```

```

## Usage

### Commands

- **Generate C Programs**:

  ```bash
  ./query_generator generate
  ```

- **Compile Directory**:  
  First, run initial compilation to generate object files for the test cases:

  ```bash
  ./query_generator compile <directory_path>
  ```

- **ReFuzz the C code directory**:
  ```bash
  ./query_generator refuzz <directory_path> [--model=<model_name>]
  ```

The default model we are using is `llama3.2`
