#include <iostream>
#include <vector>

int main() {
    // With -O3 flag
    std::cout << __builtin_expect(10, 7) << std::endl;  // __builtin_expect is used to give hint to compiler about expected value of expression.
    int a = 5, b = 2;
    if (a > b) {
        std::cout << "a is greater than b" << std::endl;
    } else {
        std::cout << "b is greater than or equal to a" << std::endl;
    }
    
    // Using #if directive
    #if 5 % 3 == 0
        std::cout << "__builtin_expect(10, 7)" << std::endl;  
    #else
        std::cout << "Condition is not met." << std::endl;
    #endif
    
    // IPSCCCP optimizations part of the compiler Parse (i.e. inlining)
    int x = 5;
    int y = 2;
    if (x > y) {
        int z = x + y;
        std::cout << "z is: " << z << std::endl;  
    }
    
    return 0;
}