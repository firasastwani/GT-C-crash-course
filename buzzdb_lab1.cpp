/**
 * =============================================================================
 * BuzzDB Lab 1: Flat-File Social Media Application
 * Georgia Tech - C++ Systems Programming
 * =============================================================================
 * 
 * LEARNING GUIDE FOR C++ (Coming from Java/Python/C)
 * ---------------------------------------------------
 * 
 * KEY C++ CONCEPTS YOU'LL LEARN:
 * 
 * 1. HEADERS vs IMPORTS:
 *    - C++ uses #include instead of import/from
 *    - <iostream> for console I/O (like System.out in Java)
 *    - <fstream> for file I/O
 *    - <string> for std::string (safer than C's char*)
 *    - <vector> is like ArrayList in Java or list in Python
 *    - <map> is like TreeMap in Java or dict in Python (but ordered)
 *    - <unordered_map> is like HashMap in Java (faster, unordered)
 * 
 * 2. NAMESPACES:
 *    - std:: prefix is like Java's java.util. prefix
 *    - We use "using namespace std;" to avoid typing std:: everywhere
 *    - In production code, it's better to be explicit with std::
 * 
 * 3. REFERENCES (&) vs POINTERS (*):
 *    - Reference (int& x): An alias to existing variable, cannot be null
 *    - Pointer (int* x): Holds memory address, can be null
 *    - Coming from C, references are like pointers that auto-dereference
 *    - Coming from Java, references are like object references but for primitives too
 * 
 * 4. const KEYWORD:
 *    - const int x = 5; // x cannot be changed (like final in Java)
 *    - const std::string& s // read-only reference (efficient, no copy)
 * 
 * 5. RAII (Resource Acquisition Is Initialization):
 *    - Objects clean up automatically when they go out of scope
 *    - No garbage collector like Java/Python - deterministic destruction
 *    - fstream files close automatically when the object is destroyed
 * 
 * 6. STL CONTAINERS:
 *    - std::vector<T>: Dynamic array (like ArrayList<T> in Java)
 *    - std::map<K,V>: Ordered map (like TreeMap<K,V> in Java)
 *    - std::unordered_map<K,V>: Hash map (like HashMap<K,V> in Java)
 *    - std::pair<A,B>: Tuple of 2 elements
 * 
 * 7. THREADING:
 *    - std::thread for creating threads (like Thread in Java)
 *    - std::mutex for locks (like synchronized in Java)
 *    - std::lock_guard for RAII-style locking
 * 
 * =============================================================================
 */

#include <iostream>      // For std::cout, std::cerr (console output)
#include <fstream>       // For std::ifstream, std::ofstream (file I/O)
#include <sstream>       // For std::stringstream (string parsing)
#include <string>        // For std::string
#include <vector>        // For std::vector (dynamic array)
#include <map>           // For std::map (ordered key-value store)
#include <unordered_map> // For std::unordered_map (hash table)
#include <algorithm>     // For std::sort, std::remove_if
#include <mutex>         // For std::mutex (thread synchronization)
#include <thread>        // For std::thread (multithreading)
#include <chrono>        // For timing measurements
#include <cstdio>        // For std::rename (atomic file rename)
#include <utility>       // For std::pair, std::move

// Using the standard namespace to avoid typing std:: everywhere
// NOTE: In production code, it's better to be explicit with std::
using namespace std;

/**
 * =============================================================================
 * DATA STRUCTURES
 * =============================================================================
 * 
 * These structs represent the data model. In C++, structs are like classes
 * but with public access by default.
 * 
 * COMPARISON TO OTHER LANGUAGES:
 * - Java: Similar to a POJO (Plain Old Java Object) or record
 * - Python: Similar to a dataclass or namedtuple
 * - C: Like a C struct, but can have methods
 */

/**
 * User struct - represents a row in users.csv
 * 
 * C++ TIP: Unlike Java, primitive types (int) are NOT automatically 
 * initialized. Always initialize them to avoid undefined behavior.
 */
struct User {
    int id = 0;              // User's unique identifier
    string username;         // Username (unique)
    string location;         // User's location (city)
    
    // Default constructor - C++ requires this if we want to create empty User objects
    User() = default;
    
    // Parameterized constructor - convenient way to create User objects
    // The ': id(id), ...' syntax is called an initializer list (more efficient)
    User(int id, const string& username, const string& location)
        : id(id), username(username), location(location) {}
};

/**
 * Post struct - represents a row in posts.csv
 */
struct Post {
    int id = 0;              // Post's unique identifier
    string content;          // The post content/text
    string username;         // Author's username (foreign key to users)
    int views = 0;           // View count
    
    Post() = default;
    
    Post(int id, const string& content, const string& username, int views)
        : id(id), content(content), username(username), views(views) {}
};

/**
 * Engagement struct - represents a row in engagements.csv
 * 
 * An engagement is either a "like" or a "comment" on a post.
 */
struct Engagement {
    int id = 0;              // Engagement's unique identifier
    int postId = 0;          // Which post this engagement is on (foreign key)
    string username;         // Who made the engagement (foreign key to users)
    string type;             // "like" or "comment"
    string comment;          // Comment text (empty if type is "like")
    long long timestamp = 0; // Unix timestamp of when engagement was made
    
    Engagement() = default;
    
    Engagement(int id, int postId, const string& username, 
               const string& type, const string& comment, long long timestamp)
        : id(id), postId(postId), username(username), 
          type(type), comment(comment), timestamp(timestamp) {}
};

/**
 * =============================================================================
 * FLATFILE CLASS - The Main Implementation
 * =============================================================================
 * 
 * This class manages the flat-file database. It:
 * - Loads CSV files into memory
 * - Provides query methods
 * - Supports concurrent updates
 * - Ensures durability through atomic file writes
 * 
 * DESIGN PATTERN: This is similar to a Repository pattern in Java/Spring
 */
class FlatFile {
private:
    // ==========================================================================
    // MEMBER VARIABLES
    // ==========================================================================
    
    // File paths for the three CSV files
    string users_csv_path;
    string posts_csv_path;
    string engagements_csv_path;
    
    // In-memory storage for loaded data
    // Using map<int, T> gives us O(log n) lookup by ID
    // Alternative: unordered_map<int, T> for O(1) average lookup
    map<int, User> users;           // user_id -> User
    map<int, Post> posts;           // post_id -> Post
    map<int, Engagement> engagements; // engagement_id -> Engagement
    
    // ==========================================================================
    // SECONDARY INDEXES (for efficient queries)
    // ==========================================================================
    // 
    // Think of indexes like the index at the back of a textbook:
    // Instead of scanning every page (row), you look up directly.
    //
    // IMPLEMENTATION TIP: Keep these in sync with main data!
    
    unordered_map<string, int> username_to_id;  // Quick username -> user_id lookup
    // TODO: Add more indexes as needed for efficient queries
    // Example: unordered_map<int, vector<int>> user_engagements; // user_id -> [engagement_ids]
    
    // ==========================================================================
    // SYNCHRONIZATION PRIMITIVES
    // ==========================================================================
    // 
    // C++ THREADING CONCEPTS:
    // - mutex: Mutual exclusion lock. Only one thread can hold it at a time.
    // - lock_guard: RAII wrapper that locks on construction, unlocks on destruction.
    //   This ensures the lock is always released, even if an exception occurs.
    //
    // COMPARISON TO JAVA:
    // - mutex + lock_guard is like synchronized(lockObject) { ... }
    // - But more flexible - you can have multiple mutexes for finer control
    
    mutex users_mutex;       // Protects users map
    mutex posts_mutex;       // Protects posts map
    mutex engagements_mutex; // Protects engagements map
    mutex file_mutex;        // Protects file write operations
    
    // ==========================================================================
    // HELPER METHODS (private)
    // ==========================================================================
    
    /**
     * Trim whitespace from both ends of a string.
     * 
     * C++ STRING TIP: Unlike Python, there's no built-in strip() method.
     * We implement it using find_first_not_of and find_last_not_of.
     * 
     * @param s The string to trim
     * @return A new string with leading/trailing whitespace removed
     */
    static string trim(const string& s) {
        // TODO: Implement string trimming
        // 
        // HINT: Use these string methods:
        // - s.find_first_not_of(" \t\r\n") returns first non-whitespace index
        // - s.find_last_not_of(" \t\r\n") returns last non-whitespace index
        // - s.substr(start, length) extracts a substring
        // - string::npos is returned when not found (like -1 in Java)
        //
        // YOUR CODE HERE:
        return s;  // Placeholder - replace with actual implementation
    }
    
    /**
     * Parse a single CSV line into a vector of cells.
     * 
     * CSV PARSING NOTES:
     * - Cells are separated by commas
     * - Each cell should be trimmed of whitespace
     * - For simplicity, we're not handling quoted fields with commas inside
     * 
     * @param line The CSV line to parse
     * @return Vector of trimmed cell values
     */
    vector<string> parseCSVLine(const string& line) {
        vector<string> cells;
        // TODO: Implement CSV line parsing
        //
        // HINT: Use std::stringstream to split by comma:
        //   stringstream ss(line);
        //   string cell;
        //   while (getline(ss, cell, ',')) {
        //       cells.push_back(trim(cell));
        //   }
        //
        // YOUR CODE HERE:
        (void)line;  // Suppress unused parameter warning until implemented
        return cells;
    }
    
    /**
     * Safely parse a string to an integer.
     * 
     * @param s The string to parse
     * @param result Output parameter for the parsed integer
     * @return true if parsing succeeded, false otherwise
     * 
     * C++ TIP: Output parameters (int& result) are common in C++
     * They're like returning multiple values. In Java you'd return an Optional<Integer>.
     */
    bool safeParseInt(const string& s, int& result) {
        // TODO: Implement safe integer parsing
        //
        // HINT: Use std::stoi with try-catch:
        //   try {
        //       size_t pos;
        //       result = stoi(s, &pos);
        //       // Check that entire string was consumed (pos == s.length())
        //       return pos == s.length();
        //   } catch (...) {
        //       return false;
        //   }
        //
        // YOUR CODE HERE:
        (void)s; (void)result;  // Suppress unused parameter warnings until implemented
        return false;  // Placeholder
    }
    
    /**
     * Safely parse a string to a long long (for timestamps).
     */
    bool safeParseLongLong(const string& s, long long& result) {
        // TODO: Similar to safeParseInt but use std::stoll
        //
        // YOUR CODE HERE:
        (void)s; (void)result;  // Suppress unused parameter warnings until implemented
        return false;  // Placeholder
    }
    
    /**
     * Load a single CSV file based on type.
     * This is a helper for both single-threaded and parallel loading.
     * 
     * @param type 0 = users, 1 = posts, 2 = engagements
     * @param local_users Temporary storage for users (used in parallel loading)
     * @param local_posts Temporary storage for posts
     * @param local_engagements Temporary storage for engagements
     */
    void loadSingleFile(int type, 
                        map<int, User>& local_users,
                        map<int, Post>& local_posts,
                        map<int, Engagement>& local_engagements) {
        // Suppress unused parameter warnings until implemented
        (void)type; (void)local_users; (void)local_posts; (void)local_engagements;
        
        // TODO: Implement file loading based on type
        //
        // STEPS:
        // 1. Determine which file path to use based on type
        // 2. Open the file with ifstream
        // 3. Skip the header line
        // 4. For each line:
        //    a. Skip empty lines
        //    b. Parse the CSV line
        //    c. Validate the row has the right number of columns
        //    d. Parse numeric fields strictly
        //    e. Create the appropriate struct and add to the local map
        //
        // FILE I/O EXAMPLE:
        //   ifstream file(path);
        //   if (!file.is_open()) {
        //       cerr << "Failed to open file: " << path << endl;
        //       return;
        //   }
        //   string line;
        //   getline(file, line);  // Skip header
        //   while (getline(file, line)) {
        //       // Process each line
        //   }
        //   // File closes automatically when 'file' goes out of scope (RAII!)
        //
        // YOUR CODE HERE:
    }
    
    /**
     * Atomically write a CSV file using temp file + rename pattern.
     * 
     * DURABILITY CONCEPT:
     * - Write to a temp file first
     * - Use rename() which is atomic on most filesystems
     * - This ensures readers never see a partial/corrupt file
     * 
     * @param path The target file path
     * @param header The CSV header line
     * @param lines The data lines to write
     * @return true if write succeeded, false otherwise
     */
    bool atomicWriteCSV(const string& path, const string& header, 
                        const vector<string>& lines) {
        // Suppress unused parameter warnings until implemented
        (void)path; (void)header; (void)lines;
        
        // TODO: Implement atomic file writing
        //
        // STEPS:
        // 1. Create a temp file path (e.g., path + ".tmp")
        // 2. Write header and all lines to temp file
        // 3. Close the file (or let RAII handle it)
        // 4. Use std::rename(temp_path.c_str(), path.c_str()) to atomically replace
        // 5. Return true on success
        //
        // HINT: std::rename returns 0 on success
        //
        // YOUR CODE HERE:
        return false;  // Placeholder
    }
    
    /**
     * Rebuild secondary indexes from main data.
     * Call this after loading data or after modifications.
     */
    void rebuildIndexes() {
        // TODO: Rebuild all secondary indexes
        //
        // Example for username_to_id:
        //   username_to_id.clear();
        //   for (const auto& [id, user] : users) {
        //       username_to_id[user.username] = id;
        //   }
        //
        // C++ TIP: The syntax 'const auto& [id, user]' is called structured bindings
        // It's like destructuring in JavaScript/Python
        //
        // YOUR CODE HERE:
    }

public:
    // ==========================================================================
    // CONSTRUCTOR & DESTRUCTOR
    // ==========================================================================
    
    /**
     * Constructor - Store file paths and initialize internal state.
     * 
     * C++ TIP: The member initializer list (after the colon) is more efficient
     * than assignment in the constructor body because it initializes directly.
     * 
     * @param users_csv_path Path to users.csv
     * @param posts_csv_path Path to posts.csv
     * @param engagements_csv_path Path to engagements.csv
     */
    FlatFile(string users_csv_path, string posts_csv_path, string engagements_csv_path)
        : users_csv_path(std::move(users_csv_path)),   // std::move avoids copying
          posts_csv_path(std::move(posts_csv_path)),
          engagements_csv_path(std::move(engagements_csv_path)) {
        // TODO: Any additional initialization
        //
        // For now, the member initializer list handles storing the paths.
        // You might want to validate the paths or pre-allocate data structures.
        //
        // YOUR CODE HERE (optional):
    }
    
    /**
     * Destructor - Clean up resources.
     * 
     * C++ TIP: The = default syntax tells the compiler to generate
     * the default destructor. Since we're using STL containers (which 
     * manage their own memory), we don't need custom cleanup.
     * 
     * If you had raw pointers (int* p = new int), you'd need to delete them here.
     * But modern C++ prefers smart pointers (unique_ptr, shared_ptr) instead.
     */
    ~FlatFile() = default;
    
    // ==========================================================================
    // CORE METHODS TO IMPLEMENT
    // ==========================================================================
    
    /**
     * Load all CSV files into memory (single-threaded).
     * 
     * REQUIREMENTS:
     * - Ignore empty lines
     * - Trim whitespace from cells
     * - Parse numeric fields strictly (reject malformed values)
     * - Skip malformed rows
     * - Build secondary indexes after loading
     */
    void loadFlatFile() {
        // TODO: Implement single-threaded loading
        //
        // STEPS:
        // 1. Clear existing data
        // 2. Load users.csv
        // 3. Load posts.csv
        // 4. Load engagements.csv
        // 5. Rebuild indexes
        //
        // You can reuse loadSingleFile() or implement directly here.
        //
        // YOUR CODE HERE:
    }
    
    /**
     * Load all CSV files in parallel (one thread per file).
     * 
     * REQUIREMENTS:
     * - Load each file in a separate thread
     * - Use local storage per thread, then merge atomically
     * - Must be faster than single-threaded on multi-core systems
     * - Validate referential integrity (no dangling references)
     * 
     * THREADING EXAMPLE:
     *   // Create threads
     *   thread t1([&]() { ... lambda body ... });
     *   thread t2([&]() { ... lambda body ... });
     *   
     *   // Wait for completion
     *   t1.join();
     *   t2.join();
     * 
     * C++ LAMBDA SYNTAX:
     *   [capture](params) { body }
     *   [&] captures all local variables by reference
     *   [=] captures all local variables by value (copy)
     */
    void loadMultipleFlatFilesInParallel() {
        // TODO: Implement parallel loading
        //
        // STEPS:
        // 1. Clear existing data
        // 2. Create local storage for each thread
        // 3. Launch 3 threads, each loading one file into local storage
        // 4. Join all threads
        // 5. Under locks, merge local storage into main maps
        // 6. Validate referential integrity
        // 7. Rebuild indexes
        //
        // YOUR CODE HERE:
    }
    
    /**
     * Update the view count of a post (thread-safe and durable).
     * 
     * @param post_id The ID of the post to update
     * @param views_count The number of views to ADD to the current count
     * @return true if update succeeded, false if post_id doesn't exist
     * 
     * REQUIREMENTS:
     * - Thread-safe (multiple threads may call this concurrently)
     * - Durable (persist to CSV immediately using atomic write)
     */
    bool updatePostViews(int post_id, int views_count) {
        // Suppress unused parameter warnings until implemented
        (void)post_id; (void)views_count;
        
        // TODO: Implement thread-safe, durable view count update
        //
        // STEPS:
        // 1. Lock the posts mutex
        // 2. Check if post_id exists
        // 3. Update the view count in memory
        // 4. Rewrite posts.csv atomically
        // 5. Return true on success
        //
        // LOCKING EXAMPLE:
        //   lock_guard<mutex> lock(posts_mutex);  // Locks on construction
        //   // ... do work ...
        //   // Automatically unlocks when lock goes out of scope
        //
        // YOUR CODE HERE:
        return false;  // Placeholder
    }
    
    /**
     * Add a new engagement record.
     * 
     * @param record The engagement to add (id will be assigned)
     * 
     * REQUIREMENTS:
     * - Validate foreign key constraints (postId must exist, username must exist)
     * - Thread-safe
     * - Append to CSV file durably
     */
    void addEngagementRecord(Engagement& record) {
        // Suppress unused parameter warnings until implemented
        (void)record;
        
        // TODO: Implement engagement addition
        //
        // STEPS:
        // 1. Validate postId exists in posts
        // 2. Validate username exists in users
        // 3. Assign a new engagement ID
        // 4. Lock and add to engagements map
        // 5. Append to engagements.csv
        // 6. Update indexes
        //
        // YOUR CODE HERE:
    }
    
    /**
     * Get all comments made by a specific user.
     * 
     * @param user_id The user's ID
     * @return Vector of (postId, comment) pairs, sorted by (postId, comment)
     */
    vector<pair<int, string>> getAllUserComments(int user_id) {
        // Suppress unused parameter warnings until implemented
        (void)user_id;
        
        // TODO: Implement comment retrieval
        //
        // STEPS:
        // 1. Find the user by ID
        // 2. Filter engagements where username matches and type == "comment"
        // 3. Collect (postId, comment) pairs
        // 4. Sort by postId, then by comment
        // 5. Return the result
        //
        // SORTING EXAMPLE:
        //   sort(result.begin(), result.end());  // pair<int,string> sorts by first, then second
        //
        // YOUR CODE HERE:
        return {};  // Placeholder - empty vector
    }
    
    /**
     * Count engagements (likes and comments) for all users in a location.
     * 
     * @param location The location to query
     * @return Pair of (likes_count, comments_count)
     */
    pair<int, int> getAllEngagementsByLocation(string location) {
        // Suppress unused parameter warnings until implemented
        (void)location;
        
        // TODO: Implement location-based engagement counting
        //
        // STEPS:
        // 1. Find all users in the given location
        // 2. For each user, count their engagements by type
        // 3. Sum up likes and comments separately
        // 4. Return {likes_count, comments_count}
        //
        // YOUR CODE HERE:
        return {0, 0};  // Placeholder
    }
    
    /**
     * Update a user's username across all CSV files.
     * 
     * @param user_id The user's ID
     * @param new_username The new username
     * @return true if update succeeded, false if user_id doesn't exist
     * 
     * REQUIREMENTS:
     * - Update in all three files (users, posts, engagements)
     * - Atomic rewrites for durability
     * - Thread-safe
     */
    bool updateUserName(int user_id, string new_username) {
        // Suppress unused parameter warnings until implemented
        (void)user_id; (void)new_username;
        
        // TODO: Implement username update
        //
        // STEPS:
        // 1. Check if user_id exists
        // 2. Get the old username
        // 3. Update in users map
        // 4. Update in posts map (where author matches)
        // 5. Update in engagements map
        // 6. Rewrite all three CSV files atomically
        // 7. Update indexes
        //
        // YOUR CODE HERE:
        return false;  // Placeholder
    }
    
    // ==========================================================================
    // ACCESSOR METHODS (for testing)
    // ==========================================================================
    
    size_t getUserCount() const { return users.size(); }
    size_t getPostCount() const { return posts.size(); }
    size_t getEngagementCount() const { return engagements.size(); }
    
    // Check if a user exists by ID
    bool hasUser(int id) const { return users.count(id) > 0; }
    
    // Check if a post exists by ID
    bool hasPost(int id) const { return posts.count(id) > 0; }
    
    // Get a post's view count (returns -1 if not found)
    int getPostViews(int post_id) const {
        auto it = posts.find(post_id);
        return it != posts.end() ? it->second.views : -1;
    }
    
    // Get a user's username (returns empty string if not found)
    string getUsername(int user_id) const {
        auto it = users.find(user_id);
        return it != users.end() ? it->second.username : "";
    }
};

// =============================================================================
// TEST CASES
// =============================================================================
// 
// These tests validate your implementation. Run with:
//   ./buzzdb_lab1.out [test_number]
// 
// Run all tests:
//   ./buzzdb_lab1.out
// 
// Run specific test:
//   ./buzzdb_lab1.out 1

/**
 * Test 1: Single-threaded load
 * Verifies that loadFlatFile correctly loads all CSV data.
 */
void test1_single_threaded_load() {
    cout << "=== Test 1: Single-threaded Load ===" << endl;
    
    FlatFile db("users.csv", "posts.csv", "engagements.csv");
    db.loadFlatFile();
    
    bool passed = true;
    
    // Check counts
    if (db.getUserCount() != 5) {
        cerr << "FAIL: Expected 5 users, got " << db.getUserCount() << endl;
        passed = false;
    }
    if (db.getPostCount() != 5) {
        cerr << "FAIL: Expected 5 posts, got " << db.getPostCount() << endl;
        passed = false;
    }
    if (db.getEngagementCount() != 8) {
        cerr << "FAIL: Expected 8 engagements, got " << db.getEngagementCount() << endl;
        passed = false;
    }
    
    // Check specific records
    if (!db.hasUser(1) || !db.hasUser(5)) {
        cerr << "FAIL: Missing expected users" << endl;
        passed = false;
    }
    if (db.getUsername(1) != "alice") {
        cerr << "FAIL: User 1 should be alice, got " << db.getUsername(1) << endl;
        passed = false;
    }
    
    if (passed) {
        cout << "PASS: All single-threaded load checks passed!" << endl;
    }
    cout << endl;
}

/**
 * Test 2: Parallel load (should be faster than single-threaded)
 */
void test2_parallel_load() {
    cout << "=== Test 2: Parallel Load ===" << endl;
    
    // Time single-threaded load
    auto start1 = chrono::high_resolution_clock::now();
    FlatFile db1("users.csv", "posts.csv", "engagements.csv");
    db1.loadFlatFile();
    auto end1 = chrono::high_resolution_clock::now();
    auto single_time = chrono::duration_cast<chrono::microseconds>(end1 - start1).count();
    
    // Time parallel load
    auto start2 = chrono::high_resolution_clock::now();
    FlatFile db2("users.csv", "posts.csv", "engagements.csv");
    db2.loadMultipleFlatFilesInParallel();
    auto end2 = chrono::high_resolution_clock::now();
    auto parallel_time = chrono::duration_cast<chrono::microseconds>(end2 - start2).count();
    
    cout << "Single-threaded: " << single_time << " microseconds" << endl;
    cout << "Parallel: " << parallel_time << " microseconds" << endl;
    
    // Verify data integrity
    bool passed = true;
    if (db2.getUserCount() != db1.getUserCount()) {
        cerr << "FAIL: User counts don't match" << endl;
        passed = false;
    }
    if (db2.getPostCount() != db1.getPostCount()) {
        cerr << "FAIL: Post counts don't match" << endl;
        passed = false;
    }
    if (db2.getEngagementCount() != db1.getEngagementCount()) {
        cerr << "FAIL: Engagement counts don't match" << endl;
        passed = false;
    }
    
    if (passed) {
        cout << "PASS: Parallel load produces correct data!" << endl;
        // Note: On small files, parallel might not be faster due to thread overhead
        // The test should pass with larger files
    }
    cout << endl;
}

/**
 * Test 3: Get all user comments
 */
void test3_user_comments() {
    cout << "=== Test 3: Get All User Comments ===" << endl;
    
    FlatFile db("users.csv", "posts.csv", "engagements.csv");
    db.loadFlatFile();
    
    // Get comments for user 2 (bob)
    auto comments = db.getAllUserComments(2);
    
    bool passed = true;
    
    // Bob has 1 comment: on post 4 "I love Atlanta too"
    if (comments.size() != 1) {
        cerr << "FAIL: Expected 1 comment for bob, got " << comments.size() << endl;
        passed = false;
    } else if (comments[0].first != 4 || comments[0].second != "I love Atlanta too") {
        cerr << "FAIL: Unexpected comment content" << endl;
        passed = false;
    }
    
    // Get comments for user 4 (diana) - 1 comment on post 2
    comments = db.getAllUserComments(4);
    if (comments.size() != 1) {
        cerr << "FAIL: Expected 1 comment for diana, got " << comments.size() << endl;
        passed = false;
    }
    
    if (passed) {
        cout << "PASS: User comments retrieval works correctly!" << endl;
    }
    cout << endl;
}

/**
 * Test 4: Engagements by location
 */
void test4_engagements_by_location() {
    cout << "=== Test 4: Engagements by Location ===" << endl;
    
    FlatFile db("users.csv", "posts.csv", "engagements.csv");
    db.loadFlatFile();
    
    // Atlanta users: alice (user 1) and eve (user 5)
    // alice has: 1 like (on post 2)
    // eve has: 1 like (on post 3)
    // Total: 2 likes, 0 comments
    auto [likes, comments] = db.getAllEngagementsByLocation("Atlanta");
    
    bool passed = true;
    if (likes != 2) {
        cerr << "FAIL: Expected 2 likes for Atlanta, got " << likes << endl;
        passed = false;
    }
    if (comments != 0) {
        cerr << "FAIL: Expected 0 comments for Atlanta, got " << comments << endl;
        passed = false;
    }
    
    // Boston: bob (user 2)
    // bob has: 1 like (on post 1), 1 comment (on post 4)
    tie(likes, comments) = db.getAllEngagementsByLocation("Boston");
    if (likes != 1 || comments != 1) {
        cerr << "FAIL: Expected 1 like, 1 comment for Boston, got " 
             << likes << " likes, " << comments << " comments" << endl;
        passed = false;
    }
    
    if (passed) {
        cout << "PASS: Engagements by location works correctly!" << endl;
    }
    cout << endl;
}

/**
 * Test 5: Update post views
 */
void test5_update_views() {
    cout << "=== Test 5: Update Post Views ===" << endl;
    
    FlatFile db("users.csv", "posts.csv", "engagements.csv");
    db.loadFlatFile();
    
    bool passed = true;
    
    int initial_views = db.getPostViews(1);
    cout << "Initial views for post 1: " << initial_views << endl;
    
    // Update views
    bool result = db.updatePostViews(1, 50);
    if (!result) {
        cerr << "FAIL: updatePostViews returned false for valid post" << endl;
        passed = false;
    }
    
    int new_views = db.getPostViews(1);
    if (new_views != initial_views + 50) {
        cerr << "FAIL: Expected " << (initial_views + 50) << " views, got " << new_views << endl;
        passed = false;
    }
    
    // Try to update non-existent post
    result = db.updatePostViews(999, 10);
    if (result) {
        cerr << "FAIL: updatePostViews should return false for non-existent post" << endl;
        passed = false;
    }
    
    if (passed) {
        cout << "PASS: Post view updates work correctly!" << endl;
    }
    cout << endl;
}

/**
 * Test 6: Update username
 */
void test6_update_username() {
    cout << "=== Test 6: Update Username ===" << endl;
    
    FlatFile db("users.csv", "posts.csv", "engagements.csv");
    db.loadFlatFile();
    
    bool passed = true;
    
    // Update alice -> alice_new
    bool result = db.updateUserName(1, "alice_new");
    if (!result) {
        cerr << "FAIL: updateUserName returned false for valid user" << endl;
        passed = false;
    }
    
    if (db.getUsername(1) != "alice_new") {
        cerr << "FAIL: Username not updated correctly" << endl;
        passed = false;
    }
    
    // Counts should remain the same
    if (db.getUserCount() != 5) {
        cerr << "FAIL: User count changed after rename" << endl;
        passed = false;
    }
    
    if (passed) {
        cout << "PASS: Username update works correctly!" << endl;
    }
    cout << endl;
}

/**
 * Test 7: Add engagement record
 */
void test7_add_engagement() {
    cout << "=== Test 7: Add Engagement Record ===" << endl;
    
    FlatFile db("users.csv", "posts.csv", "engagements.csv");
    db.loadFlatFile();
    
    bool passed = true;
    
    size_t initial_count = db.getEngagementCount();
    
    // Add a new engagement
    Engagement newEngagement(0, 1, "eve", "like", "", 1706500000);
    db.addEngagementRecord(newEngagement);
    
    if (db.getEngagementCount() != initial_count + 1) {
        cerr << "FAIL: Engagement count didn't increase" << endl;
        passed = false;
    }
    
    if (passed) {
        cout << "PASS: Engagement addition works correctly!" << endl;
    }
    cout << endl;
}

/**
 * Test 8: Concurrent view updates
 */
void test8_concurrent_updates() {
    cout << "=== Test 8: Concurrent View Updates ===" << endl;
    
    FlatFile db("users.csv", "posts.csv", "engagements.csv");
    db.loadFlatFile();
    
    int initial_views = db.getPostViews(1);
    int num_threads = 10;
    int updates_per_thread = 10;
    
    vector<thread> threads;
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&db]() {
            for (int j = 0; j < 10; j++) {
                db.updatePostViews(1, 1);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    int final_views = db.getPostViews(1);
    int expected_views = initial_views + (num_threads * updates_per_thread);
    
    if (final_views == expected_views) {
        cout << "PASS: Concurrent updates correctly applied! Views: " << final_views << endl;
    } else {
        cerr << "FAIL: Expected " << expected_views << " views, got " << final_views << endl;
        cerr << "This indicates a race condition in updatePostViews" << endl;
    }
    cout << endl;
}

/**
 * Main function - runs tests
 */
int main(int argc, char* argv[]) {
    cout << "========================================" << endl;
    cout << "BuzzDB Lab 1 - Flat File Social Media" << endl;
    cout << "========================================" << endl << endl;
    
    // If a test number is provided, run only that test
    if (argc > 1) {
        int test_num = atoi(argv[1]);
        switch (test_num) {
            case 1: test1_single_threaded_load(); break;
            case 2: test2_parallel_load(); break;
            case 3: test3_user_comments(); break;
            case 4: test4_engagements_by_location(); break;
            case 5: test5_update_views(); break;
            case 6: test6_update_username(); break;
            case 7: test7_add_engagement(); break;
            case 8: test8_concurrent_updates(); break;
            default:
                cerr << "Unknown test number: " << test_num << endl;
                cerr << "Valid tests: 1-8" << endl;
                return 1;
        }
    } else {
        // Run all tests
        test1_single_threaded_load();
        test2_parallel_load();
        test3_user_comments();
        test4_engagements_by_location();
        test5_update_views();
        test6_update_username();
        test7_add_engagement();
        test8_concurrent_updates();
        
        cout << "========================================" << endl;
        cout << "All tests completed!" << endl;
        cout << "========================================" << endl;
    }
    
    return 0;
}
