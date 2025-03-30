#include <pthread.h>
#include <stdlib.h>
#include "sat.h"

typedef int (*BoolFun)(int x[]);

typedef struct {
  int n;
  int p;
  BoolFun f;
  int k;
  int* count;
} ThreadData;

int gen_seq(int x[], int i, int n, BoolFun f) {
  if (i == n) {
    return (*f)(x);
  } else {
    int count = 0;
    x[i] = 0;
    count += gen_seq(x, i + 1, n, f);
    x[i] = 1;
    count += gen_seq(x, i + 1, n, f);
    return count;
  }
}

void* thread_func(void* arg) {
  ThreadData* data = (ThreadData*)arg;
  int n = data->n;
  int p = data->p;
  BoolFun f = data->f;
  int k = data->k;
  int* count = data->count;
  
  int x[n];
  for (int j = 0; j < p; j++) {
    x[j] = (k >> j) & 1;
  }
  int local_count = gen_seq(x, p, n, f);
  *count = local_count;
  return NULL;
}

int recuento(int n, BoolFun f, int p) {
  int num_threads = 1 << p;
  pthread_t threads[num_threads];
  ThreadData data[num_threads];
  int counts[num_threads];
  
  for (int k = 0; k < num_threads; k++) {
    data[k].n = n;
    data[k].p = p;
    data[k].f = f;
    data[k].k = k;
    data[k].count = &counts[k];
    pthread_create(&threads[k], NULL, thread_func, &data[k]);
  }
  
  for (int k = 0; k < num_threads; k++) {
    pthread_join(threads[k], NULL);
  }
  
  int total = 0;
  for (int k = 0; k < num_threads; k++) {
    total += counts[k];
  }
  return total;
}
