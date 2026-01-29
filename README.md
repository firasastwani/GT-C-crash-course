# BuzzDB Lab 1 - Flat-File Social Media Application

A C++ systems programming exercise from Georgia Tech's database course.

## Quick Start

### Compile
```bash
g++ -fdiagnostics-color -std=c++17 -O3 -Wall -Werror -Wextra buzzdb_lab1.cpp -o buzzdb_lab1.out
```

### Run All Tests
```bash
./buzzdb_lab1.out
```

### Run Specific Test
```bash
./buzzdb_lab1.out 1    # Run test 1 only
./buzzdb_lab1.out 5    # Run test 5 only
```

---

## Project Overview

You're building a tiny social media application that stores data in three CSV files:
- **users.csv**: User accounts (id, username, location)
- **posts.csv**: Social media posts (id, content, username, views)
- **engagements.csv**: Likes and comments (id, postId, username, type, comment, timestamp)

---

## C++ Concepts You'll Learn (Coming from Java/Python/C)

### 1. Memory Model & RAII

**Java/Python**: Garbage collector cleans up objects automatically (non-deterministic)
**C++**: Objects are destroyed when they go out of scope (deterministic)

```cpp
void example() {
    ifstream file("data.txt");  // File opens here
    // ... use file ...
}  // File automatically closes here (RAII)
```

This is called **RAII (Resource Acquisition Is Initialization)**. It's one of C++'s most powerful patterns.

### 2. References vs Pointers

| Feature | Reference (`int& x`) | Pointer (`int* x`) |
|---------|---------------------|-------------------|
| Can be null? | No | Yes |
| Can be reassigned? | No | Yes |
| Syntax | Use like normal variable | Need `*` to dereference |
| Java equivalent | Object references | N/A (closest: Optional) |
| C equivalent | N/A | Same as C |

```cpp
void increment(int& x) {   // Reference - modifies original
    x++;
}

void incrementPtr(int* x) { // Pointer - need to dereference
    (*x)++;
}

int main() {
    int val = 5;
    increment(val);        // val is now 6
    incrementPtr(&val);    // val is now 7
}
```

### 3. const Correctness

```cpp
// Immutable value
const int MAX_SIZE = 100;

// Efficient read-only parameter (no copy!)
void process(const string& data) {
    // data cannot be modified
}

// Const method - promises not to modify object state
class Example {
    int getValue() const { return value; }
};
```

### 4. STL Containers

| C++ | Java | Python | Use Case |
|-----|------|--------|----------|
| `vector<T>` | `ArrayList<T>` | `list` | Dynamic array |
| `map<K,V>` | `TreeMap<K,V>` | `dict` (sorted) | Ordered key-value |
| `unordered_map<K,V>` | `HashMap<K,V>` | `dict` | Fast key-value |
| `pair<A,B>` | `Map.Entry` | `tuple` | Two values |

```cpp
// Vector (like ArrayList)
vector<int> nums = {1, 2, 3};
nums.push_back(4);
for (int n : nums) cout << n << " ";

// Map (like TreeMap - ordered)
map<string, int> ages;
ages["alice"] = 25;
ages["bob"] = 30;

// Unordered map (like HashMap - faster)
unordered_map<int, string> id_to_name;
id_to_name[1] = "alice";
```

### 5. Iterating Over Containers

```cpp
// C++17 structured bindings (like JS destructuring)
for (const auto& [key, value] : myMap) {
    cout << key << ": " << value << endl;
}

// Traditional iterator
for (auto it = myMap.begin(); it != myMap.end(); ++it) {
    cout << it->first << ": " << it->second << endl;
}
```

### 6. Threading

```cpp
#include <thread>
#include <mutex>

mutex data_mutex;

void thread_safe_update(int& data) {
    lock_guard<mutex> lock(data_mutex);  // Locks here
    data++;
    // Automatically unlocks when lock goes out of scope
}

int main() {
    vector<thread> threads;
    
    // Launch threads
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([](){ /* work */ });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
}
```

### 7. File I/O

```cpp
#include <fstream>
#include <sstream>

// Reading
ifstream infile("data.csv");
string line;
while (getline(infile, line)) {
    stringstream ss(line);
    string cell;
    while (getline(ss, cell, ',')) {
        // Process each cell
    }
}

// Writing
ofstream outfile("output.csv");
outfile << "id,name,value" << endl;
outfile << "1,alice,100" << endl;
```

---

## Implementation Guide

### Phase 1: Helper Functions

Start with these utility functions:

1. **`trim()`** - Remove whitespace from string ends
2. **`parseCSVLine()`** - Split CSV line into cells
3. **`safeParseInt()`** - Parse integers with error handling

### Phase 2: Loading (Single-threaded)

Implement `loadFlatFile()`:
1. Open each CSV file
2. Skip header line
3. Parse each data line
4. Create structs and add to maps

### Phase 3: Query Methods

Implement in order:
1. `getAllUserComments()` - Filter and sort engagements
2. `getAllEngagementsByLocation()` - Aggregate counts

### Phase 4: Mutations

These require thread safety:
1. `updatePostViews()` - Increment + atomic file write
2. `addEngagementRecord()` - Validate + append
3. `updateUserName()` - Update all files

### Phase 5: Parallel Loading

Implement `loadMultipleFlatFilesInParallel()`:
1. Load into thread-local storage
2. Merge under locks
3. Validate referential integrity

---

## Common Patterns

### Atomic File Updates (Durability)

```cpp
bool atomicWriteFile(const string& path, const string& content) {
    string temp_path = path + ".tmp";
    
    // Write to temp file
    ofstream out(temp_path);
    out << content;
    out.close();
    
    // Atomic rename
    return rename(temp_path.c_str(), path.c_str()) == 0;
}
```

### Thread-Safe Updates

```cpp
bool updateValue(int id, int delta) {
    lock_guard<mutex> lock(data_mutex);
    
    auto it = data.find(id);
    if (it == data.end()) return false;
    
    it->second.value += delta;
    
    // Persist to disk
    return writeToFile();
}
```

### Checking Existence

```cpp
// Using count (returns 0 or 1 for map)
if (users.count(user_id) > 0) { ... }

// Using find (more flexible)
auto it = users.find(user_id);
if (it != users.end()) {
    User& user = it->second;
    // Use user
}
```

---

## Debugging Tips

### Compiler Errors
- Read from **bottom to top** - root cause is often at the bottom
- Template errors are verbose - look for your file/line first

### Runtime Debugging
- Use `cerr` for debug output (unbuffered, goes to stderr)
- Print state before/after operations
- Use `-g` flag for debug symbols: `g++ -g ...`

### Common Mistakes
1. Forgetting to initialize primitive types (`int x = 0;`)
2. Returning reference to local variable (dangling reference)
3. Iterator invalidation (modifying container while iterating)
4. Not handling empty strings/edge cases

---

## Test Descriptions

| Test | Description |
|------|-------------|
| 1 | Single-threaded CSV loading - verify counts and data |
| 2 | Parallel loading - must be faster, same results |
| 3 | User comments query - filter and sort engagements |
| 4 | Location engagement counts - aggregation query |
| 5 | Update post views - thread-safe increment |
| 6 | Update username - cascading update across files |
| 7 | Add engagement - validate foreign keys |
| 8 | Concurrent updates - thread safety stress test |

---

## File Structure

```
GT C++/
├── buzzdb_lab1.cpp      # Your implementation (skeleton)
├── users.csv            # Sample user data
├── posts.csv            # Sample post data
├── engagements.csv      # Sample engagement data
├── README.md            # This file
└── ACTIVITY_LOG.md      # Track your progress
```

---

## Resources

- [C++ Reference](https://en.cppreference.com/) - Standard library documentation
- [Learn C++](https://www.learncpp.com/) - Comprehensive tutorials
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/) - Best practices

Good luck with your implementation!
# GT-C-crash-course
