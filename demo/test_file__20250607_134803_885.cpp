#include <iostream>
#include <cstdlib>
#include <cstring>

const int SIZE = 10; // Define the size of the array to be sorted
int arr[SIZE] = { 3, 2, 7, 5, 9, 4, 1, 6, 8 }; // Initialize the array with some values

// Define a sorting function using bubble sort
void sortArr(int arr[], int size) {
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

int main() {
    sortArr(arr, SIZE); // Sort the array using the sorting function
    for (int i : arr) {
        std::cout << i << " ";
    }
    return 0;
}