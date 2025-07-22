#include <iostream>
#include <algorithm>
#include <numeric>

#define __STRICT_ANSI__

void printArray(int arr[], int size) {
    for (int i = 0; i < size; ++i)
        std::cout << arr[i] << " ";
}

void sortArray(int arr[], int size) {
    std::sort(arr, arr + size);
    printArray(arr, size);
}

bool isGreater(int a, int b) { return a > b; }

int main() {
    // Example array
    int numbers[] = {12, 45, 7, 23, 56, 89, 34};
    int size = sizeof(numbers) / sizeof(numbers[0]);

    // Print original array
    std::cout << "Original array: ";
    printArray(numbers, size);
    std::cout << std::endl;

    // Sort the array using std::sort
    sortArray(numbers, size);

    // Example for isGreater macro
    if (isGreater(10, 20))
        std::cout << "10 is greater than 20" << std::endl;
    else
        std::cout << "10 is not greater than 20" << std::endl;

    return 0;
}