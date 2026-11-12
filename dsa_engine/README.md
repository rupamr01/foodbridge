# FoodBridge DSA Lab (standalone C++)

This folder is a separate, testable C++ program for the DSA part of FoodBridge.
It does **not** change or depend on the Flask app, MySQL, or Supabase.

The goal is to demonstrate standard data structures and algorithms with a
food-donation theme, using only material that is easy to explain in a viva.
Each structure and algorithm is kept in its own `.cpp` file so the codebase is
easy to read and present.

## Build

Windows PowerShell:

```powershell
cd dsa_engine
.\build.ps1
```

Linux/macOS:

```bash
cd dsa_engine
bash build.sh
```

The project uses **C++14** because the installed compiler is MinGW GCC 6.3.
No CMake is required. The build scripts compile every file in `src/`.

## Run

Windows:

```powershell
.\foodbridge_dsa.exe
```

Linux/macOS:

```bash
./foodbridge_dsa
```

Run the automatic tests without opening the menu:

```powershell
.\foodbridge_dsa.exe --selftest
```

Expected result:

```text
Result: 36 passed, 0 failed.
```

The self-tests cover every data structure, every sorting/searching routine,
the date helpers, and the best-match scoring rules.

## What each menu option demonstrates

| Option | Feature | Data structure / algorithm |
|---|---|---|
| 1 | Show all listings | Singly linked list traversal |
| 2 | Add listing | Linked list insert + hash map + BST insert |
| 3 | Remove listing | Linked list delete + hash map erase + BST rebuild |
| 4 | Search by ID | Hash map (`std::unordered_map`) |
| 5 | Search by ID | Binary search tree |
| 6 | Search by ID | Merge sort + binary search |
| 7 | Search by name | Linear search |
| 8 | Sort by expiry | Merge sort |
| 9 | Sort by quantity | Insertion sort |
| 10 | Sort by name | Quick sort |
| 11 | Expiring soon | Binary min-heap |
| 12 | Add NGO request | Circular queue enqueue |
| 13 | Process next request | Circular queue dequeue |
| 14 | Show pending requests | Queue traversal |
| 15 | Undo last added listing | LIFO stack |
| 16 | Category/donor totals | Hash map aggregation |
| 17 | BST in-order traversal | In-order traversal |
| 18 | BFS pickup network | Graph adjacency list + BFS |
| 19 | DFS pickup network | Graph adjacency list + DFS |
| 20 | Self-tests | Automated checks for all structures |
| 21 | Best match for an NGO | Weighted score + merge sort |

## Best match (option 21)

The NGO enters:

- its latitude and longitude,
- the quantity it needs,
- an optional preferred category.

The program first removes expired or empty listings. It then scores every
remaining listing from 0 to 100:

```text
score = 45% distance
      + 30% expiry urgency
      + 15% quantity fit
      + 10% category match
```

Each part is a value from 0.0 to 1.0:

| Part | Rule | Easy explanation |
|---|---|---|
| Distance | `max(0, 1 - km / 20)` | Food at the NGO's location gets full marks; 20 km or farther gets zero. |
| Urgency | `1 - min(daysLeft, 14) / 14` | Food expiring today gets full marks; 14 or more days left gets zero. |
| Quantity | `min(1, available / requested)` | Enough food gets full marks; a short quantity gets a proportional score. |
| Category | `1` for a match, `0` otherwise | A blank category counts as a match for every listing. |

Finally, the results are ordered with the hand-written **merge sort**
(`MergeSort.cpp`). Ties are broken by earlier expiry, then shorter distance,
then lower food ID. This is rule-based matching, not machine learning.

Example: an NGO near Connaught Place needs 20 meals. Fresh Bread is very close,
expires tomorrow, and has 25 available, so it ranks first. Packaged Snacks are
farther away and not urgent, so they rank lower even though they have plenty of
quantity.

## Files

```text
dsa_engine/
├── include/
│   ├── Food.h              # Food and NGO request records + makeFood()
│   ├── DateUtils.h         # Date validation and calendar arithmetic
│   ├── MatchResult.h       # One scored best-match result
│   ├── BestMatch.h         # Best-match weights and function declarations
│   ├── FoodStore.h         # Keeps list + hash map + BST in sync
│   ├── SeedData.h          # Sample data loader
│   ├── SelfTests.h         # Self-test entry point
│   ├── SinglyLinkedList.h  # FoodList declarations
│   ├── Stack.h             # FoodStack declarations
│   ├── Queue.h             # RequestQueue declarations
│   ├── MinHeap.h           # FoodMinHeap declarations
│   ├── BST.h               # FoodBST declarations
│   ├── Sorting.h           # Merge/quick/insertion sort declarations
│   ├── Searching.h         # Linear/binary search declarations
│   └── Graph.h             # Graph + BFS/DFS + distance declarations
├── src/
│   ├── main.cpp            # Console menu
│   ├── SinglyLinkedList.cpp
│   ├── Stack.cpp
│   ├── Queue.cpp
│   ├── MinHeap.cpp
│   ├── BST.cpp
│   ├── MergeSort.cpp
│   ├── QuickSort.cpp
│   ├── InsertionSort.cpp
│   ├── LinearSearch.cpp
│   ├── BinarySearch.cpp
│   ├── Graph.cpp
│   ├── DateUtils.cpp
│   ├── BestMatch.cpp
│   ├── FoodStore.cpp
│   ├── SeedData.cpp
│   └── SelfTests.cpp
├── build.ps1
├── build.sh
├── README.md
└── VIVA_NOTES.md
```

## Complexity summary

| Operation | Structure / algorithm | Average / normal case |
|---|---|---|
| Add listing at end | Linked list | O(n) |
| Search by ID | Hash map | O(1) average |
| Search by ID | BST | O(log n) average, O(n) worst |
| Search by ID | Binary search | O(log n) after O(n log n) sort |
| Search by name | Linear search | O(n) |
| Merge sort | Array/vector | O(n log n) |
| Quick sort | Array/vector | O(n log n) average, O(n²) worst |
| Insertion sort | Array/vector | O(n²) |
| Expiring soon | Min-heap | O(log n) push/pop |
| Add/process request | Queue | O(1) |
| Undo | Stack | O(1) push/pop |
| BFS / DFS | Graph | O(V + E) |
| Best match | Score every listing + merge sort | O(n log n) |

## How this maps to the real FoodBridge project

The Flask app stores food listings in MySQL and orders in MySQL. This lab uses
the same concepts in memory:

- **Food listing** = `Food` struct
- **NGO request** = `FoodRequest` struct
- **Browse listings** = linked list traversal
- **Search by ID** = hash map / BST / binary search
- **Search by name** = linear search
- **Expiring soon** = min-heap ordered by `best_before`
- **Pending NGO requests** = FIFO queue
- **Undo last listing** = stack
- **Nearby pickup points** = graph; BFS/DFS shows connected pickup areas
- **Admin totals** = hash map counting quantities by category and donor
- **Best match** = weighted score + merge sort

Later, the same C++ core can be connected to Flask through a small Python
bridge. The website will send the current MySQL rows to the C++ program and get
ranked results back. Until then, this folder can be built and demonstrated on
its own.

## Simple viva explanation

1. Food listings are stored in a **singly linked list**. Adding a node costs
   O(n) because we walk to the end; searching and deleting are O(n).
2. The **hash map** stores the same food by ID, so an ID lookup is O(1) on
   average.
3. The **BST** also stores food by ID. Searching follows left/right branches,
   so the average cost is O(log n). In-order traversal prints IDs in sorted
   order.
4. **Merge sort** divides the list in half, sorts each half, and merges them.
   It is stable and always O(n log n).
5. **Quick sort** chooses a pivot, partitions the data, and sorts the two
   parts. It is O(n log n) on average, but O(n²) in the worst case.
6. **Insertion sort** is simple and good for small data. It is O(n²).
7. **Binary search** only works on sorted data. It halves the search range
   each time, so it is O(log n).
8. The **min-heap** keeps the earliest `best_before` date at the root. Pushing
   and popping are O(log n).
9. The **queue** is FIFO: the first NGO request added is the first one
   processed.
10. The **stack** is LIFO: the last listing added is the first one undone.
11. The **graph** connects pickup points within 1.5 km. BFS uses a queue and
    visits nodes level by level; DFS uses a stack and goes deep first. Both are
    O(V + E).
12. **Best match** multiplies four simple 0–1 scores by fixed weights, adds
    them, and sorts the results with merge sort. There is no greedy algorithm
    and no machine learning.

## Notes

- Sample data uses dates relative to today, so the demo never starts with all
  food expired.
- Coordinates are around Delhi, India, and are only for the graph and distance
  demos.
- The graph uses straight-line haversine distance. It is not a real road route.
- This is a teaching lab. The Flask app remains the source of truth for real
  accounts, listings, and orders.
