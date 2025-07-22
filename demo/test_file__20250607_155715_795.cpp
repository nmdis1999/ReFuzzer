#include <iostream>
#include <cstdlib>
#include <cstring>
#include <ctime>

// Define a struct to hold the data being sorted
struct Sortable {
    int value;
    char name[20]; // Maximum length of name is 20 characters
};

// Function to sort the data in ascending order
void sortData() {
    Sortable data[10] = {
        {5, "Alice"},
        {3, "Bob"},
        {7, "Charlie"},
        {2, "David"},
        {8, "Eve"},
        {4, "Frank"},
        {6, "Grace"},
        {9, "Helen"},
        {10, "Ivan"}
    };

    // Sort the data using a recursive compare function
    sortRecursive(data, 10);
}

// Recursive compare function to sort the data
void sortRecursive(Sortable arr[], int n) {
    if (n == 1) return; // Base case: only one element, no need to sort
    int mid = n / 2; // Split the array into two halves
    Sortable left = {arr[0], arr[1]}; // First half of the array
    Sortable right = {arr[mid + 1], arr[n - 1]}; // Second half of the array
    sortRecursive(left, mid); // Recursively sort the first half
    sortRecursive(right, n - mid); // Recursively sort the second half
    // Now compare and swap the elements of the two halves
    int i = 0;
    while (i < left.n) {
        if (left.value < right.value) {
            arr[i] = left;
            i++;
        } else {
            arr[i] = right;
            i++;
        }
    }
}

int main() {
    srand(time(NULL)); // Generate some random data
    Sortable data[10]; // Create an array of 10 Sortable objects
    for (int i = 0; i < 10; i++) {
        data[i] = ({rand() % 10, "Name" + std::to_string(i)});
    }
    sortData(); // Sort the data
    return 0;
}