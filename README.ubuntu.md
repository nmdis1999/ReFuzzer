Setup Installations:
```
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh all

sudo apt update
sudo apt install -y python3 python3-pip python3-dev build-essential gcc g++ gdb make cmake valgrind clang clang-format clang-tidy
sudo apt install -y libcurl4-openssl-dev nlohmann-json3-dev pkg-config
sudo apt install -y llvm

for version in 15 16 17; do
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

```

System Specifications
```
Operating System: Ubuntu 22.04.2 LTS
Kernel: Linux 5.15.0-122-generic
CPU: AMD EPYC 7452 32-Core
Architecture: x86_64
```

Clang versions used:

```
Ubuntu clang version 16.0.6 (++20231112100510+7cbf1a259152-1~exp1~20231112100554.106)
Target: x86\_64-pc-linux-gnu
Thread model: posix
InstalledDir: /usr/bin

Ubuntu clang version 15.0.7
Target: x86_64-pc-linux-gnu
Thread model: posix
InstalledDir: /usr/bin:W

Ubuntu clang version 17.0.6 (++20231209124227+6009708b4367-1~exp1~20231209124336.77)
Target: x86_64-pc-linux-gnu
Thread model: posix
InstalledDir: /usr/bin

```


