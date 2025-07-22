#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cmath>

// Define a struct to hold the data to be sorted
struct SortExample { int value; std::string name; };

// Function to sort the data in place
void sortData(SortExample arr[], size_t numElements) {
  // Use the qsort algorithm to sort the data
  qsort(arr, numElements, sizeof(SortExample),
        [](const SortExample& a, const SortExample& b) {
          return a.value < b.value;
        });
}

int main() {
  // Define some sample data to be sorted
  SortExample arr[5] = {
    {1, "apple"}, {3, "banana"}, {2, "orange"}, {4, "grape"}, {6, "watermelon"}
  };
  
  // Sort the data in place using qsort
  size_t numElements = sizeof(arr) / sizeof(arr[0]);
  sortData(arr, numElements);
  
  // Print the sorted data
  for (const auto& element : arr) {
    std::cout << element.name << ": " << element.value << std::endl;
  }
  
  return 0;
}