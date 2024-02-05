#include <iostream>
#include <utility>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <algorithm>
#include <random>


using namespace std;

typedef pair<int,int> ii;

/*
This function performs the merge operation of merge sort.

Parameters:
array : vector<int> - array to sort
s     : int         - start index of merge
e     : int         - end index (inclusive) of merge
*/
void merge(vector<int> &array, int s, int e);

class Task {
public:
    ii range;
    Task* fc; // First child dependency
    Task* sc; // Second child dependency
    bool done; // If this task is done sorting

    Task(ii range): range(range), fc(nullptr), sc(nullptr), done(false) {}

    void execute(vector<int> &array) {
        merge(array, range.first, range.second);
        finish();
    }

    void execute_sequential(vector<int> &array) {
        merge(array, range.first, range.second);
    }

    void finish() {
        done = true;
    }
};

// Each thread will run this function to get work from the queue
void execute_task(vector<int>& array, queue<Task*>& ready_queue, mutex& queue_mutex, bool& done, vector<Task*>& done_tasks, mutex& done_tasks_mutex) {
    while (true) {
        Task* task = nullptr;    
        { // Mutex block
            unique_lock<mutex> lock(queue_mutex);
            // If the program is done. The program is done when all tasks are done
            if (done) {
                return;
            }

            // If there is a task in the queue, pop it and assign it to task
            if (!ready_queue.empty()) {
                task = ready_queue.front();
                ready_queue.pop();
            }
        }

        if (task != nullptr) {
            bool noDependencies = task->fc == nullptr && task->sc == nullptr;
            bool dependenciesDone = task->fc != nullptr && task->fc->done && (task->sc == nullptr || (task->sc != nullptr && task->sc->done));

            if (noDependencies || dependenciesDone) {
                task->execute(array);
                {
                    unique_lock<mutex> lock(done_tasks_mutex);
                    done_tasks.push_back(task);
                }
            } else {
                // If dependencies are not done, re-queue the task
                {
                    unique_lock<mutex> lock(queue_mutex);
                    ready_queue.push(task);
                }
            }
        }
    }
}

// This function returns a goes one step down a branch and checks the left and right branch
Task generate_intervals_recursive(int start, int end, vector<Task>& tasks) {
    if (start == end) {
        return Task(ii(-1, -1)); // return a -1,-1 if node has no children dependencies
    }

    Task task(ii(start, end));
    int mid = start + (end - start) / 2;

    // Left branch
    Task left_task = generate_intervals_recursive(start, mid, tasks);
    // Track any dependencies or children for the left interval
    if (left_task.range.first != -1) {
        tasks.push_back(left_task); // Save the left task pointer
        task.fc = &tasks.back(); // Make the task in the tasks list a dependent
    }

    // Right branch
    Task right_task = generate_intervals_recursive(mid + 1, end, tasks);
    if (right_task.range.first != -1) {
        tasks.push_back(right_task); // Save the right task pointer
        task.sc = &tasks.back(); // Make the task in the tasks list a dependent
    }

    return task;
}

void generate_intervals(int start, int end, vector<Task>& tasks) {
    Task root = generate_intervals_recursive(start, end, tasks);
    tasks.push_back(root);
}

void merge(vector<int> &array, int s, int e) {
    int m = s + (e - s) / 2;
    vector<int> left;
    vector<int> right;
    for(int i = s; i <= e; i++) {
        if(i <= m) {
            left.push_back(array[i]);
        } else {
            right.push_back(array[i]);
        }
    }
    int l_ptr = 0, r_ptr = 0;

    for(int i = s; i <= e; i++) {
        // no more elements on left half
        if(l_ptr == (int)left.size()) {
            array[i] = right[r_ptr];
            r_ptr++;

        // no more elements on right half or left element comes first
        } else if(r_ptr == (int)right.size() || left[l_ptr] <= right[r_ptr]) {
            array[i] = left[l_ptr];
            l_ptr++;
        } else {
            array[i] = right[r_ptr];
            r_ptr++;
        }
    }
}

int main(){
    int arraySize, threadCount; // user input
    bool done = false; // track when the program is done
    vector<thread> workers; 
    queue<Task*> task_queue; // used by threads to get work
    mutex queue_mutex; // when accessing the task queue
    mutex done_tasks_mutex; // when accessing the done tasks list
    vector<Task> tasks; // tasks generated from intervals
    vector<Task*> done_tasks; // used to not destroy the pointers to tasks
    vector<int> array; // The array to sort

    // TODO: Seed your randomizer
    mt19937 rng(42);

    // TODO: Get array size and thread count from user
    arraySize = 8388608;
    threadCount = 4;
    cout << "Enter the size of the array: ";
    cin >> arraySize;
    cout << "Enter the number of threads: ";
    cin >> threadCount;

    //cout << "Thread count: " << threadCount << endl;

    array.reserve(arraySize);
    tasks.reserve(arraySize);
    done_tasks.reserve(arraySize);
    uniform_int_distribution<int> dist(1, arraySize);

    // TODO: Generate a random array of given size
    for (int i = 0; i < arraySize; ++i) {
        array.push_back(i + 1);
    }
    
    // Shuffle the array
    shuffle(array.begin(), array.end(), rng);

    // start del
    // vector<int> arr = array;
    // cout << "Sequential Initial Array: " << endl;
    // for (int num : array) {
    //     cout << num << " ";
    // }
    // cout << endl;
    // end del

    // TODO: Call the generate_intervals method to generate the merge sequence
    generate_intervals(0, arraySize - 1, tasks); // adds interval to tasks vector
/*
    // Start timer for sequential
    auto start = chrono::high_resolution_clock::now();

    // Sequential. TODO: Call merge on each interval in sequence
    for (Task& task : tasks) {
        task.execute_sequential(array);
    }

    // End timer for sequential
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> duration = end - start;

    // Sanity check sort
    bool isSorted = is_sorted(array.begin(), array.end());
    
    cout << "Array is " << (isSorted ? "sorted" : "not sorted") << endl;
    cout << "Sequential execution time: " << duration.count() << " seconds" << endl;
*/

    // ------- BEGIN CONCURRENT VERSION ----------------------------------------------

    // Shuffle array again using same seed
    rng.seed(42);
    shuffle(array.begin(), array.end(), rng);
/*
    // start del
     cout << endl << "Concurrent Initial Array: " << endl;
     for (int num : array) {
         cout << num << " ";
     }
     cout << endl;
    // end del
*/
    // Start timer for concurrent
    auto start = chrono::high_resolution_clock::now();

    // Add all tasks to the queue
    for (Task& task : tasks) {
        task_queue.push(&task);
    }
    
    // Create a pool of threads to get work from the queue of tasks
    for (int i = 0; i < threadCount; i++) {
        workers.push_back(thread(execute_task, ref(array), ref(task_queue), ref(queue_mutex), ref(done), ref(done_tasks), ref(done_tasks_mutex)));
    }

    // Keep track if the program is done by checking if all tasks are done
    while (!done) {
        // Check if all tasks are done
        bool all_done = true;
        for (Task& task : tasks) {
            if (!task.done) {
                all_done = false;
                break;
            }
        }

        if (all_done) {
            done = true;
        }
    }

    // Join all threads and end the program properly
    for (thread& worker : workers) {
        worker.join();
    }

    // End timer
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> duration = end - start;
/*
     cout << endl << "Concurrent Resulting Array: " << endl;
     for (int num : array) {
         cout << num << " ";
     }
     cout << endl;
*/
    // Sanity check sort
    auto isSorted = is_sorted(array.begin(), array.end());
    
    cout << "Array is " << (isSorted ? "sorted" : "not sorted") << endl;
    cout << "Concurrent execution time: " << duration.count() << " seconds" << endl;

    return 0;
}

