#include <pthread.h>
#include <stdio.h>
#include "sat.h"

// Structure to pass data to each thread
typedef struct {
    int k;          // Thread identifier (0 to 2^p - 1)
    int n;          // Total number of variables
    int p;          // Number of variables to fix (log2 of thread count)
    BoolFun f;      // Boolean function to evaluate
    int* resultado; // Pointer to where the thread writes its result
} ThreadData;

// Recursive function to generate combinations and count true evaluations
int gen(int x[], int i, int n, BoolFun f)
{
    if (i == n) {
        return f(x); // Base case: evaluate f(x) when all variables are set
    } else {
        int cnt = 0;
        x[i] = 0;
        cnt += gen(x, i + 1, n, f); // Set x[i] to 0 and recurse
        x[i] = 1;
        cnt += gen(x, i + 1, n, f); // Set x[i] to 1 and recurse
        return cnt;
    }
}

// Thread function to process a subset of combinations
void* thread_func(void* arg)
{
    ThreadData* data = (ThreadData*)arg;
    int k = data->k;
    int n = data->n;
    int p = data->p;
    BoolFun f = data->f;
    int* resultado = data->resultado;
    int x[n]; // Each thread has its own copy of x

    // Set the first p variables based on k's binary representation
    for (int i = 0; i < p; i++) {
        x[i] = (k >> i) & 1;
    }

    // Count true evaluations for remaining variables
    int cnt = gen(x, p, n, f);
    *resultado = cnt; // Store result in designated location
    return NULL;
}

// Main function to compute the count using 2^p threads
int recuento(int n, BoolFun f, int p)
{
    int num_threads = 1 << p; // 2^p threads
    pthread_t threads[num_threads];
    ThreadData data[num_threads];
    int resultados[num_threads];

    // Initialize and launch threads
    for (int k = 0; k < num_threads; k++) {
        data[k].k = k;
        data[k].n = n;
        data[k].p = p;
        data[k].f = f;
        data[k].resultado = &resultados[k];
        if (num_threads == 1) {
            // For p = 0, run directly to avoid thread overhead
            thread_func(&data[k]);
        } else {
            pthread_create(&threads[k], NULL, thread_func, &data[k]);
        }
    }

    // Collect results
    int total = 0;
    for (int k = 0; k < num_threads; k++) {
        if (num_threads > 1) {
            pthread_join(threads[k], NULL); // Wait for thread to finish
        }
        total += resultados[k];
    }
    return total;
}
