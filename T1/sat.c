#include <pthread.h>
#include <stdio.h>
#include "sat.h"

// Structure to pass arguments to the thread function
typedef struct {
    int x[20];     // Reduced to n <= 20 to fit stack
    int start_i;   // Starting index after fixed prefix
    int n;         // Total number of variables
    BoolFun f;     // Boolean function to evaluate
    int result;    // Store the result of this thread
} ThreadArg;

// Recursive function to count true evaluations sequentially
static int gen(int x[], int i, int n, BoolFun f) {
    if (i == n) {
        return f(x);  // Base case: evaluate f
    }
    int count = 0;
    x[i] = 0;
    count += gen(x, i + 1, n, f);
    x[i] = 1;
    count += gen(x, i + 1, n, f);
    return count;
}

// Thread function to process a subset of combinations
void* thread_gen(void* arg) {
    ThreadArg* t_arg = (ThreadArg*)arg;
    t_arg->result = gen(t_arg->x, t_arg->start_i, t_arg->n, t_arg->f);
    pthread_exit(NULL);
}

// Main function to count true evaluations using 2^p threads
int recuento(int n, BoolFun f, int p) {
    if (p >= n) p = n;  // Limit p to n
    if (p > 3) p = 3;   // Cap at 2^3 = 8 threads to avoid stack overflow
    int num_threads = 1 << p;

    // Pre-allocate on stack, reduced to 8 threads max
    int x_arrays[8][20];       // 8 threads, n <= 20
    ThreadArg args[8];         // 8 thread arguments
    pthread_t threads[8];      // 8 thread handles

    // Check bounds
    if (n > 20 || p > 3) {
        fprintf(stderr, "n > 20 or p > 3 not supported without malloc\n");
        return -1;
    }

    // Initialize each thread's x array with a unique p-bit prefix
    for (int t = 0; t < num_threads; t++) {
        for (int i = 0; i < p; i++) {
            x_arrays[t][i] = (t >> (p - 1 - i)) & 1;
        }
        for (int i = p; i < n; i++) {
            x_arrays[t][i] = 0;
        }

        // Set up thread arguments
        for (int i = 0; i < n; i++) {
            args[t].x[i] = x_arrays[t][i];
        }
        args[t].start_i = p;
        args[t].n = n;
        args[t].f = f;

        if (pthread_create(&threads[t], NULL, thread_gen, &args[t]) != 0) {
            perror("pthread_create failed");
            return -1;
        }
    }

    // Collect results
    int total_count = 0;
    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
        total_count += args[t].result;
    }

    return total_count;
}
