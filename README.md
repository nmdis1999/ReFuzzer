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
- Curl
- nlohmann / json

Installing prerequisites :
```
sudo apt install -y nlohmann-json3-dev curl
```
## Building the Project

1. Clone the repository:

```bash
git clone https://github.com/nmdis1999/ReFuzzer.git
cd ReFuzzer
```

2. Set up Conan and install dependencies:

```bash
conan profile detect
```
if the command doesn't work, try updating the path in your desired profile.
e.g `export PATH="$HOME/.local/bin:$PATH"` and then reload the profile.
```
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

Pull latest refuzzer image:
```
docker pull shreei/refuzzer:latest
```
Run the container with the image:
```
docker run -p 11434:11434 shreei/refuzzer:latest
```
and finally connect to the container
```
docker exec -it <container_id> /bin/bash
```

## Usage

### Commands

You can find the binary to run in src/query_generator folder.
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
