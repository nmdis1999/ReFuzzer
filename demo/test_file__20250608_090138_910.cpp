#include <iostream>

int main() {
    char str[] = "Hello, World!";
    std::cout << strnlen_s(str, sizeof(str)) << std::endl; // should print the length of the string "Hello, World!"
    return 0;
}