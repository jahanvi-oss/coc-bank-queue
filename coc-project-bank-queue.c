

#include <stdio.h>
#include <stdlib.h> // For malloc, free, realloc, rand, srand, qsort
#include <math.h>   // For exp, sqrt, pow (for Poisson and Std Dev)
#include <time.h>   // For srand(time(NULL))
#include <string.h> // For memset (used for mode calculation)

// --- Simulation Constants ---
#define SIMULATION_MINUTES 480 // 8 hours * 60 minutes
#define MIN_SERVICE_TIME 2     // Minimum minutes to serve a customer
#define MAX_SERVICE_TIME 3     // Maximum minutes to serve a customer
#define INITIAL_STORAGE_CAPACITY 100 // Initial size for our dynamic wait-time array

/*
 * ============================================================================
 * 1. STRUCT DEFINITIONS
 * ============================================================================
 */

/**
 * @brief A single customer in the queue. This is a node in our linked list.
 */
typedef struct Customer
{
    int arrival_minute;     // The minute the customer entered the queue
    struct Customer *next;  // Pointer to the next customer in line
} Customer;

/**
 * @brief The queue manager. Holds pointers to the front and rear of the
 * linked list, allowing for O(1) enqueue and dequeue operations.
 */
typedef struct Queue
{
    Customer *front; // Pointer to the head of the list
    Customer *rear;  // Pointer to the tail of the list
    int customer_count;
} Queue;

/**
 * @brief Represents a single bank teller.
 */
typedef struct Teller
{
    int is_busy;                // 0 = free, 1 = busy
    int remaining_service_time; // Minutes left until this teller is free
} Teller;

/**
 * @brief A dynamic array to store the wait times of all *served* customers.
 * This will grow as needed using realloc().
 */
typedef struct WaitTimeStorage
{
    int *wait_times; // Pointer to the dynamically allocated array of wait times
    int count;       // Current number of wait times stored
    int capacity;    // Current total capacity of the array
} WaitTimeStorage;

/*
 * ============================================================================
 * 2. QUEUE MANAGEMENT FUNCTIONS (Linked List Implementation)
 * ============================================================================
 */

/**
 * @brief Creates and initializes a new, empty queue.
 * @return Pointer to the newly allocated Queue.
 */
Queue *create_queue()
{
    // Allocate memory for the queue manager struct
    Queue *q = (Queue *)malloc(sizeof(Queue));
    if (q == NULL)
    {
        perror("Failed to allocate memory for queue");
        exit(EXIT_FAILURE);
    }
    q->front = NULL;
    q->rear = NULL;
    q->customer_count = 0;
    return q;
}

/**
 * @brief Checks if the queue is empty.
 * @return 1 (true) if empty, 0 (false) if not.
 */
int is_empty(Queue *q)
{
    return (q->front == NULL);
}

/**
 * @brief Adds a new customer to the REAR of the queue.
 * @param q The queue to modify.
 * @param arrival_minute The simulation minute the customer arrived.
 */
void enqueue(Queue *q, int arrival_minute)
{
    // 1. Allocate memory for the new customer (node)
    Customer *new_customer = (Customer *)malloc(sizeof(Customer));
    if (new_customer == NULL)
    {
        perror("Failed to allocate memory for new customer");
        exit(EXIT_FAILURE);
    }
    new_customer->arrival_minute = arrival_minute;
    new_customer->next = NULL;

    // 2. Link the new customer to the end of the list
    if (is_empty(q))
    {
        // If queue is empty, new customer is both front and rear
        q->front = new_customer;
        q->rear = new_customer;
    }
    else
    {
        // Otherwise, link the current rear to the new customer
        q->rear->next = new_customer;
        // And update the rear to be the new customer
        q->rear = new_customer;
    }
    q->customer_count++;
}

/**
 * @brief Removes and returns the customer from the FRONT of the queue.
 * Returns NULL if the queue is empty.
 * @param q The queue to modify.
 * @return Pointer to the removed Customer (or NULL).
 */
Customer *dequeue(Queue *q)
{
    // 1. Check if queue is empty
    if (is_empty(q))
    {
        return NULL;
    }

    // 2. Get the customer at the front
    Customer *served_customer = q->front;

    // 3. Move the front pointer to the next customer
    q->front = q->front->next;

    // 4. If the queue is now empty, update the rear pointer as well
    if (q->front == NULL)
    {
        q->rear = NULL;
    }

    q->customer_count--;
    return served_customer; // The calling function is responsible for free()-ing this customer
}

/**
 * @brief Frees all remaining customers in the queue and the queue itself.
 */
void free_queue(Queue *q)
{
    Customer *current = q->front;
    while (current != NULL)
    {
        Customer *temp = current;
        current = current->next;
        free(temp);
    }
    free(q);
}

/*
 * ============================================================================
 * 3. DYNAMIC ARRAY (WaitTimeStorage) FUNCTIONS
 * ============================================================================
 */

/**
 * @brief Creates and initializes a new, empty storage for wait times.
 * @return Pointer to the newly allocated WaitTimeStorage.
 */
WaitTimeStorage *create_storage()
{
    WaitTimeStorage *storage = (WaitTimeStorage *)malloc(sizeof(WaitTimeStorage));
    if (storage == NULL)
    {
        perror("Failed to allocate memory for storage");
        exit(EXIT_FAILURE);
    }

    // Allocate the initial array to hold wait times
    storage->wait_times = (int *)malloc(INITIAL_STORAGE_CAPACITY * sizeof(int));
    if (storage->wait_times == NULL)
    {
        perror("Failed to allocate memory for initial wait time array");
        exit(EXIT_FAILURE);
    }

    storage->count = 0;
    storage->capacity = INITIAL_STORAGE_CAPACITY;
    return storage;
}

/**
 * @brief Adds a new wait time to the storage, resizing the array if necessary.
 * This is the "dynamic" part of the array.
 * @param storage The storage to modify.
 * @param wait_time The new wait time to add.
 */
void add_wait_time(WaitTimeStorage *storage, int wait_time)
{
    // 1. Check if the array is full
    if (storage->count == storage->capacity)
    {
        // If full, double the capacity
        int new_capacity = storage->capacity * 2;
        int *new_array = (int *)realloc(storage->wait_times, new_capacity * sizeof(int));

        if (new_array == NULL)
        {
            perror("Failed to re-allocate memory for wait time array");
            // We can't add the new time, but the old data is still valid.
            // In a real-world app, we'd handle this more gracefully.
            return;
        }

        storage->wait_times = new_array;
        storage->capacity = new_capacity;
    }

    // 2. Add the new wait time
    storage->wait_times[storage->count] = wait_time;
    storage->count++;
}

/**
 * @brief Frees the dynamic array and the storage struct itself.
 */
void free_storage(WaitTimeStorage *storage)
{
    free(storage->wait_times);
    free(storage);
}

/*
 * ============================================================================
 * 4. SIMULATION & MATH FUNCTIONS
 * ============================================================================
 */

/**
 * @brief Generates a random number of customer arrivals for a given minute
 * using the Poisson distribution (Knuth's algorithm).
 * @param lambda The average number of arrivals per minute.
 * @return The (random) number of customers (k) who arrived this minute.
 */
int get_poisson_random(double lambda)
{
    // This algorithm is a standard, efficient way to generate
    // Poisson-distributed random numbers.
    double L = exp(-lambda);
    double p = 1.0;
    int k = 0;

    do
    {
        k++;
        // Get a random float between 0.0 and 1.0
        double u = (double)rand() / RAND_MAX;
        p *= u;
    } while (p > L);

    return k - 1;
}

/**
 * @brief Gets a random service time for a customer.
 * @return A random integer between MIN_SERVICE_TIME and MAX_SERVICE_TIME.
 */
int get_service_time()
{
    // (rand() % (MAX - MIN + 1)) + MIN
    return (rand() % (MAX_SERVICE_TIME - MIN_SERVICE_TIME + 1)) + MIN_SERVICE_TIME;
}

/*
 * ============================================================================
 * 5. DATA ANALYSIS FUNCTIONS
 * ============================================================================
 */

/**
 * @brief A comparison function required by qsort() to sort integers.
 */
int compare_int(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

/**
 * @brief Calculates the mean (average) of the wait times.
 */
double get_mean(int *data, int n)
{
    if (n == 0) return 0.0;
    long long sum = 0; // Use long long to prevent overflow
    for (int i = 0; i < n; i++)
    {
        sum += data[i];
    }
    return (double)sum / n;
}

/**
 * @brief Calculates the median (middle value) of the wait times.
 * @note This function ASSUMES the data array has already been sorted.
 */
double get_median(int *sorted_data, int n)
{
    if (n == 0) return 0.0;
    
    if (n % 2 == 0)
    {
        // Even number of elements: average the two middle ones
        int mid1 = sorted_data[n / 2 - 1];
        int mid2 = sorted_data[n / 2];
        return (double)(mid1 + mid2) / 2.0;
    }
    else
    {
        // Odd number of elements: return the middle one
        return (double)sorted_data[n / 2];
    }
}

/**
 * @brief Calculates the mode (most frequent value) of the wait times.
 * Uses a frequency array for efficiency.
 */
int get_mode(int *data, int n)
{
    if (n == 0) return 0;

    // We need to find the max wait time to size our frequency array.
    // (We could also just use the max possible sim time)
    int max_val = 0;
    for (int i = 0; i < n; i++) {
        if (data[i] > max_val) max_val = data[i];
    }

    // What if the max wait time is 0 (e.g., no customers)?
    // We still need an array of at least size 1.
    int freq_array_size = (max_val < 1) ? 1 : max_val + 1;

    // Allocate a frequency array and initialize to zero
    // calloc is perfect for this, as it zeroes the memory.
    int *frequency = (int *)calloc(freq_array_size, sizeof(int));
    if (frequency == NULL) {
        perror("Failed to allocate memory for mode calculation");
        return -1; // Error
    }

    // Populate the frequency array
    for (int i = 0; i < n; i++)
    {
        frequency[data[i]]++;
    }

    // Find the index (value) with the highest frequency
    int mode = 0;
    int max_freq = 0;
    for (int i = 0; i < freq_array_size; i++)
    {
        if (frequency[i] > max_freq)
        {
            max_freq = frequency[i];
            mode = i;
        }
    }

    free(frequency); // Clean up the frequency array
    return mode;
}

/**
 * @brief Calculates the standard deviation of the wait times.
 */
double get_std_dev(int *data, int n, double mean)
{
    if (n == 0) return 0.0;
    
    double sum_sq_diff = 0.0;
    for (int i = 0; i < n; i++)
    {
        sum_sq_diff += pow(data[i] - mean, 2);
    }
    return sqrt(sum_sq_diff / n);
}

/**
 * @brief Finds the single longest wait time.
 * @note This function ASSUMES the data array has already been sorted.
 */
int get_max_wait(int *sorted_data, int n)
{
    if (n == 0) return 0;
    return sorted_data[n - 1]; // The last element of the sorted array
}

/*
 * ============================================================================
 * 6. MAIN SIMULATION FUNCTION
 * ============================================================================
 */

void run_simulation(double lambda, int num_tellers)
{
    printf("\n--- Starting 8-Hour (480 Minute) Simulation ---\n");
    printf("     Avg. Arrivals / Min (Lambda): %.2f\n", lambda);
    printf("     Number of Tellers: %d\n", num_tellers);
    printf("--------------------------------------------------\n");

    // 1. --- Initialize all simulation components ---

    // Seed the random number generator
    srand(time(NULL));

    // Create the bank queue
    Queue *bank_queue = create_queue();

    // Create the dynamic array for storing wait times
    WaitTimeStorage *storage = create_storage();

    // Create the array of tellers
    Teller *tellers = (Teller *)malloc(num_tellers * sizeof(Teller));
    if (tellers == NULL) {
        perror("Failed to allocate memory for tellers");
        exit(EXIT_FAILURE);
    }
    // Initialize all tellers to be free
    for (int i = 0; i < num_tellers; i++)
    {
        tellers[i].is_busy = 0;
        tellers[i].remaining_service_time = 0;
    }

    // Statistics trackers
    int total_arrivals = 0;

    // 2. --- Run the main simulation loop ---
    for (int current_minute = 0; current_minute < SIMULATION_MINUTES; current_minute++)
    {
        // --- Step 1: Handle Tellers (Decrement service time, free them up) ---
        for (int t = 0; t < num_tellers; t++)
        {
            if (tellers[t].is_busy)
            {
                tellers[t].remaining_service_time--;
                if (tellers[t].remaining_service_time == 0)
                {
                    tellers[t].is_busy = 0;
                }
            }
        }

        // --- Step 2: Handle New Customer Arrivals ---
        int new_arrivals = get_poisson_random(lambda);
        total_arrivals += new_arrivals;
        for (int i = 0; i < new_arrivals; i++)
        {
            enqueue(bank_queue, current_minute);
        }

        // --- Step 3: Assign Free Tellers to Waiting Customers ---
        for (int t = 0; t < num_tellers; t++)
        {
            // If this teller is free AND there's someone in the queue
            if (!tellers[t].is_busy && !is_empty(bank_queue))
            {
                // 1. Dequeue the next customer
                Customer *served_customer = dequeue(bank_queue);

                // 2. Calculate and store their wait time
                int wait_time = current_minute - served_customer->arrival_minute;
                add_wait_time(storage, wait_time);

                // 3. Occupy the teller
                tellers[t].is_busy = 1;
                tellers[t].remaining_service_time = get_service_time();

                // 4. Free the customer struct (we are done with it)
                free(served_customer);
            }
        }
    } // --- End of simulation loop ---

    printf("... Simulation complete.\n\n");

    // 3. --- Post-Simulation Analysis & Report ---
    printf("========== 📊 FINAL SIMULATION REPORT 📊 ==========\n");
    printf("\n--- Simulation Summary ---\n");
    printf("Total Customers Arrived: %d\n", total_arrivals);
    printf("Total Customers Served:  %d\n", storage->count);
    printf("Customers Left in Queue: %d\n", bank_queue->customer_count);

    if (storage->count == 0)
    {
        printf("\nNo customers were served. Cannot generate wait-time statistics.\n");
    }
    else
    {
        printf("\n--- Wait Time Analysis (in minutes) ---\n");

        // Sort the data IN-PLACE. This is crucial for Median and Max.
        qsort(storage->wait_times, storage->count, sizeof(int), compare_int);

        // Calculate all statistics
        double mean = get_mean(storage->wait_times, storage->count);
        double median = get_median(storage->wait_times, storage->count);
        int mode = get_mode(storage->wait_times, storage->count);
        double std_dev = get_std_dev(storage->wait_times, storage->count, mean);
        int max_wait = get_max_wait(storage->wait_times, storage->count);

        // Print the report
        printf("Mean (Average) Wait: %.2f minutes\n", mean);
        printf("Median Wait:         %.1f minutes\n", median);
        printf("Mode Wait:           %d minutes\n", mode);
        printf("Standard Deviation:  %.2f minutes\n", std_dev);
        printf("Longest Wait Time:   %d minutes\n", max_wait);
    }
    printf("===================================================\n");


    // 4. --- Clean up all allocated memory ---
    free(tellers);
    free_queue(bank_queue);
    free_storage(storage);
}

/*
 * ============================================================================
 * 7. MAIN FUNCTION
 * ============================================================================
 */

int main()
{
    double lambda;
    int num_tellers;

    printf("--- 🏦 Welcome to the Bank Queue Simulator ---\n");
    printf("This program will simulate an 8-hour bank day.\n\n");

    // Get Lambda from user
    printf("Enter the average number of customers arriving *per minute* (lambda): ");
    if (scanf("%lf", &lambda) != 1 || lambda <= 0) {
        printf("Invalid input. Please enter a positive number.\n");
        return 1;
    }

    // Get number of tellers from user
    printf("Enter the number of tellers working: ");
    if (scanf("%d", &num_tellers) != 1 || num_tellers <= 0) {
        printf("Invalid input. Please enter a positive number of tellers.\n");
        return 1;
    }

    // Run the main simulation
    run_simulation(lambda, num_tellers);

    return 0;
}