#!/bin/bash

wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 17

sudo apt update
sudo apt install -y python3 python3-pip python3-dev build-essential gcc g++ gdb make cmake valgrind clang clang-format clang-tidy
sudo apt install -y libcurl4-openssl-dev nlohmann-json3-dev pkg-config
sudo apt install -y llvm

sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-17 1700
sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-17 1700

for version in 15 16; do
    sudo apt install -y clang-$version lldb-$version lld-$version clang-tools-$version
    sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-$version $((version * 100))
    sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-$version $((version * 100))
done

curl -fsSL https://ollama.com/install.sh | sh
mkdir -p include
curl -o include/json.hpp https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp
sleep 5

ollama pull llama2
ollama pull llama2:latest
git clone https://github.com/nmdis1999/smartcompilertesting.git


