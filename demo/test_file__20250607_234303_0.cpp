#include <cstdlib>
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace std;

int main() {
    const int N = 10; // number of elements to sort
    int arr[N] = {4, 2, 7, 1, 3, 9, 6, 8, 5}; // array to be sorted

    // Sort the array using the bubble sort algorithm
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N - i; j++) {
            if (arr[j] > arr[j + 1]) {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }

    // Print the sorted array
    for (int i : arr) {
        cout << i << " ";
    }
    cout << endl;

    return 0;
}