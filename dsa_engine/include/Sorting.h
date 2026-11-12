#pragma once

#include <vector>

#include "Food.h"
#include "MatchResult.h"

// Each sorting algorithm lives in its own .cpp file:
//   MergeSort.cpp      - merge sort
//   QuickSort.cpp      - quick sort
//   InsertionSort.cpp  - insertion sort
namespace Sorting {

// Earliest best-before date first.
void mergeSortByExpiry(std::vector<Food>& data);

// Used before binary search, which needs sorted ids.
void mergeSortById(std::vector<Food>& data);

// Highest best-match score first.
void mergeSortByMatchScore(std::vector<MatchResult>& data);

// Alphabetical order by food name.
void quickSortByName(std::vector<Food>& data);

// Smallest quantity first.
void insertionSortByQuantity(std::vector<Food>& data);

} // namespace Sorting
