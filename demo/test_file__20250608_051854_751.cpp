#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <iostream>
#include <vector>

using namespace std;

int main() {
  // Sorting example
  vector<int> nums = {3, 5, 2, 6, 1, 4};
  sort(nums.begin(), nums.end());
  for (int i : nums) {
    cout << i << " ";
  }
  cout << endl;

  // Arithmetic example
  int a = 5;
  int b = 3;
  cout << a + b << endl;

  return 0;
}