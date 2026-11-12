# FoodBridge DSA Lab — Viva Notes

Use this page for a quick explanation. The C++ program is a standalone lab
first; the Flask website can be connected to it later.

## One-line project explanation

"FoodBridge connects hotels with surplus food to NGOs. For the DSA part, I built
a C++ program that stores food listings and NGO requests in memory and uses
standard data structures to search, sort, prioritize, and rank them."

## Data structures used and why

| Structure / algorithm | File | Where it is used | Why |
|---|---|---|---|
| Singly linked list | `SinglyLinkedList.cpp` | Main list of food listings | Shows nodes, pointers, insertion, deletion, traversal |
| Hash map (`unordered_map`) | `FoodStore.cpp`, `main.cpp` | Search food by ID; count categories/donors | Average O(1) lookup and counting |
| BST | `BST.cpp` | Search food by ID; in-order sorted output | Shows tree search and in-order traversal |
| Min-heap | `MinHeap.cpp` | "Expiring soon" list | Keeps earliest `best_before` at the root |
| Circular queue | `Queue.cpp` | NGO food requests | FIFO: first request is processed first |
| Stack | `Stack.cpp` | Undo last added listing | LIFO: last added is removed first |
| Graph (adjacency list) | `Graph.cpp` | Nearby pickup points | BFS/DFS on connected pickup locations |
| Merge sort | `MergeSort.cpp` | Sort by expiry; sort best-match results | Stable O(n log n) sorting |
| Quick sort | `QuickSort.cpp` | Sort by food name | Average O(n log n) divide-and-conquer sorting |
| Insertion sort | `InsertionSort.cpp` | Sort by quantity | Simple O(n²) sorting for small data |
| Linear search | `LinearSearch.cpp` | Search food by name | Checks every item; O(n) |
| Binary search | `BinarySearch.cpp` | Search food ID in a sorted vector | Halves the range each step; O(log n) |
| Best-match score | `BestMatch.cpp` | Rank food for an NGO | Combines distance, urgency, quantity, category |
| Date helpers | `DateUtils.cpp` | Demo dates and expiry countdown | Real calendar arithmetic with YYYY-MM-DD |
| Self-tests | `SelfTests.cpp` | `--selftest` mode | Proves each structure and rule works |

STL is used only where allowed: `std::vector`, `std::unordered_map`, and the
`queue`/`stack` adapters for graph traversal. The linked list, min-heap, BST,
circular queue, sorting functions, and best-match logic are written by hand.

## Three-minute demo path

1. Run `.\\foodbridge_dsa.exe`.
2. Option 1 — show the linked list.
3. Option 4 — search ID `101` with the hash map.
4. Option 5 — search ID `103` with the BST.
5. Option 6 — search ID `105` with binary search.
6. Option 8 — show merge sort by expiry.
7. Option 11 — show the min-heap "expiring soon" order.
8. Option 12, 14, 13 — add a request, show the FIFO queue, process it.
9. Option 18 and 19 — BFS and DFS on the pickup graph.
10. Option 21 — best match for an NGO near `28.6139, 77.2090` needing `20`.
11. Option 20 — run all self-tests. Expected: **36 passed, 0 failed**.

## Likely teacher questions

### 1. Why did you use a linked list instead of an array?

A linked list stores each food in a node with a pointer to the next node. It is
good for showing insertion and deletion by changing pointers. Searching is O(n)
because we may visit every node. In a real database, an indexed table is better,
but this lab is for demonstrating the data structure.

### 2. Why is hash map search O(1)?

The key (food ID) is converted into a hash value, which points to a bucket. On
average the bucket is small, so lookup is constant time. In the worst case many
keys collide, so it can become O(n).

### 3. Why do you need a BST if the hash map is faster?

The hash map is faster for exact ID lookup, but it does not keep order. A BST
keeps keys ordered, so in-order traversal gives sorted IDs and range queries are
possible. Average search is O(log n). If the tree becomes unbalanced, the worst
case is O(n).

### 4. Why does binary search need sorted data?

Binary search compares the middle element and throws away half the array. That
only works if the data is ordered. Sorting first costs O(n log n), and each
search after that costs O(log n).

### 5. Difference between merge sort and quick sort?

Merge sort divides the array into halves, sorts them, and merges them. It is
stable and always O(n log n), but it needs extra memory. Quick sort chooses a
pivot and partitions the array in place. It is O(n log n) on average but O(n²)
in the worst case.

### 6. Why use a heap for "expiring soon"?

A heap keeps the minimum item at the root. We can push all listings in O(log n)
each and pop the earliest five in O(log n) each. Sorting everything would also
work, but a heap is the natural priority-queue structure.

### 7. Where is a stack used?

The undo feature. The last listing added is pushed on top, and undo pops it
first. That is LIFO — last in, first out.

### 8. Where is a queue used?

NGO requests. The first request added is the first one processed. That is FIFO —
first in, first out. The queue is implemented as a circular array so space is
reused.

### 9. BFS vs DFS?

BFS uses a queue and visits level by level. DFS uses a stack and goes as deep as
possible before backtracking. Both visit every vertex and edge once, so both are
O(V + E). The graph here connects pickup points within 1.5 km.

### 10. How does the best-match score work?

The NGO gives a latitude/longitude, a quantity needed, and an optional category.
Every non-expired listing gets four scores from 0.0 to 1.0:

```text
distance:  max(0, 1 - km / 20)
urgency:   1 - min(daysLeft, 14) / 14
quantity:  min(1, available / requested)
category:  1 for a match, 0 otherwise
```

Then:

```text
total = 100 * (0.45 * distance
             + 0.30 * urgency
             + 0.15 * quantity
             + 0.10 * category)
```

Finally, `MergeSort.cpp` sorts the results from highest score to lowest. Ties go
to the earlier expiry, then the shorter distance.

### 11. Why these weights?

They express the project's priority:

- 45% distance — pickup cost matters most to an NGO.
- 30% urgency — food that expires sooner should be rescued first.
- 15% quantity — enough food for the request is useful, but not as important as
  distance and expiry.
- 10% category — a preference, not a hard requirement.

The weights are named constants in `BestMatch.h`, so they are easy to change and
demonstrate.

### 12. Why no greedy algorithm or machine learning?

The goal is an explainable DSA project. A weighted score plus merge sort uses
only arithmetic and a standard sorting algorithm. There is no hidden model and
every result can be calculated by hand. A greedy algorithm is not needed because
we rank all listings instead of building a route or a schedule.

### 13. Where does this fit into FoodBridge?

The real Flask app stores accounts, food, and orders in MySQL. This lab uses the
same ideas in memory:

- Food listing → linked list node
- NGO request → queue item
- ID lookup → hash map / BST / binary search
- Expiring food → min-heap
- Nearby pickups → graph
- Best match → weighted score + merge sort

The next step is to connect the C++ engine to Flask through a small Python
bridge. The website will send current MySQL rows to C++ and receive ranked
results back.

## Limitations to mention honestly

- This is an in-memory teaching lab, not the production database.
- Sample dates are relative to today, but they are still sample data.
- The graph and best match use straight-line haversine distance, not real roads.
- The current stage is a standalone DSA module; Flask integration is the next
  step.
- The hash map and graph traversal adapters come from the STL; the custom
  structures are the linked list, BST, min-heap, circular queue, and sorts.
- "Best match" is rule-based, not artificial intelligence.

## Build and test commands

```powershell
cd dsa_engine
.\\build.ps1
.\\foodbridge_dsa.exe --selftest
.\\foodbridge_dsa.exe
```

Expected self-test result: **36 passed, 0 failed.**
