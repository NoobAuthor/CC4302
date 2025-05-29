#include <nthread-impl.h>
#include "subasta.h"
#include <stdlib.h>

// =============================================================
// Implementación del tipo Subasta
// =============================================================

typedef struct {
  nThread th;           // Thread que hace la oferta
  double price;        // Precio ofrecido
} Offer;

typedef struct {
  Offer **heap;        // Heap array para la cola de prioridad
  int size;           // Número actual de elementos
  int capacity;       // Capacidad máxima
} PriorityQueue;

struct subasta {
  int n;              // Número de unidades disponibles
  int closed;         // Indica si la subasta está cerrada
  PriorityQueue *offers;  // Cola de prioridad para las ofertas
};

// Funciones auxiliares para la cola de prioridad
static PriorityQueue* createPriorityQueue(int capacity) {
  PriorityQueue* pq = (PriorityQueue*)malloc(sizeof(PriorityQueue));
  pq->heap = (Offer**)malloc(capacity * sizeof(Offer*));
  pq->size = 0;
  pq->capacity = capacity;
  return pq;
}

static void swap(Offer **a, Offer **b) {
  Offer *temp = *a;
  *a = *b;
  *b = temp;
}

// Min-heap para mantener las ofertas más bajas en la raíz
static void heapifyUp(PriorityQueue *pq, int idx) {
  while (idx > 0) {
    int parent = (idx - 1) / 2;
    if (pq->heap[parent]->price > pq->heap[idx]->price) {
      swap(&pq->heap[parent], &pq->heap[idx]);
      idx = parent;
    } else {
      break;
    }
  }
}

static void heapifyDown(PriorityQueue *pq, int idx) {
  int minIdx = idx;
  int left = 2 * idx + 1;
  int right = 2 * idx + 2;

  if (left < pq->size && pq->heap[left]->price < pq->heap[minIdx]->price)
    minIdx = left;
  if (right < pq->size && pq->heap[right]->price < pq->heap[minIdx]->price)
    minIdx = right;

  if (idx != minIdx) {
    swap(&pq->heap[idx], &pq->heap[minIdx]);
    heapifyDown(pq, minIdx);
  }
}

static void pushOffer(PriorityQueue *pq, Offer *offer) {
  if (pq->size < pq->capacity) {
    pq->heap[pq->size] = offer;
    heapifyUp(pq, pq->size);
    pq->size++;
  }
}

static Offer* popOffer(PriorityQueue *pq) {
  if (pq->size == 0) return NULL;
  
  Offer *top = pq->heap[0];
  pq->size--;
  if (pq->size > 0) {
    pq->heap[0] = pq->heap[pq->size];
    heapifyDown(pq, 0);
  }
  return top;
}

static Offer* peekOffer(PriorityQueue *pq) {
  return pq->size > 0 ? pq->heap[0] : NULL;
}

// Implementación de las funciones principales

nSubasta nNuevaSubasta(int n) {
  nSubasta s = (nSubasta)malloc(sizeof(struct subasta));
  s->n = n;
  s->closed = 0;
  s->offers = createPriorityQueue(n * 2); // Espacio extra para manejar ofertas rechazadas
  return s;
}

int nOfrecer(nSubasta s, double precio) {
  START_CRITICAL;
  
  if (s->closed) {
    END_CRITICAL;
    return 0;
  }
  
  Offer *offer = (Offer*)malloc(sizeof(Offer));
  offer->th = nSelf();
  offer->price = precio;
  
  if (s->offers->size >= s->n) {
    // Verificar si esta oferta es mejor que la más baja actual
    Offer *lowest = peekOffer(s->offers);
    if (lowest && precio > lowest->price) {
      // Reemplazar la oferta más baja
      Offer *removed = popOffer(s->offers);
      nThread loserThread = removed->th;
      pushOffer(s->offers, offer);
      
      // Marcar el thread perdedor y despertarlo
      loserThread->retPtr = (void*)0;
      setReady(loserThread);
      
      // Suspender este thread
      suspend(WAIT_SUBASTA);
      
      free(removed);
      schedule();
      
      END_CRITICAL;
      return (int)(long long)nSelf()->retPtr;
    } else {
      free(offer);
      END_CRITICAL;
      return 0;
    }
  }
  
  // Hay espacio para una nueva oferta
  pushOffer(s->offers, offer);
  suspend(WAIT_SUBASTA);
  schedule();
  
  END_CRITICAL;
  return (int)(long long)nSelf()->retPtr;
}

double nAdjudicar(nSubasta s, int *punidades) {
  START_CRITICAL;
  
  s->closed = 1;
  double total = 0;
  int sold = 0;
  
  // Crear un arreglo temporal para ordenar las ofertas de mayor a menor
  int numOffers = s->offers->size;
  Offer **sortedOffers = (Offer**)malloc(numOffers * sizeof(Offer*));
  
  // Copiar todas las ofertas al arreglo temporal
  for (int i = 0; i < numOffers; i++) {
    sortedOffers[i] = popOffer(s->offers);
  }
  
  // Ordenar las ofertas de mayor a menor
  for (int i = 0; i < numOffers - 1; i++) {
    for (int j = 0; j < numOffers - i - 1; j++) {
      if (sortedOffers[j]->price < sortedOffers[j + 1]->price) {
        Offer *temp = sortedOffers[j];
        sortedOffers[j] = sortedOffers[j + 1];
        sortedOffers[j + 1] = temp;
      }
    }
  }
  
  // Procesar las ofertas en orden de mayor a menor
  for (int i = 0; i < numOffers; i++) {
    Offer *offer = sortedOffers[i];
    if (sold < s->n) {
      // Oferta ganadora
      total += offer->price;
      sold++;
      offer->th->retPtr = (void*)1;
    } else {
      // Oferta perdedora
      offer->th->retPtr = (void*)0;
    }
    setReady(offer->th);
    free(offer);
  }
  
  free(sortedOffers);
  
  if (punidades != NULL) {
    *punidades = s->n - sold;
  }
  
  schedule();
  END_CRITICAL;
  
  return total;
}

void nDestruirSubasta(nSubasta s) {
  START_CRITICAL;
  
  // Liberar todas las ofertas pendientes
  while (s->offers->size > 0) {
    Offer *offer = popOffer(s->offers);
    free(offer);
  }
  
  free(s->offers->heap);
  free(s->offers);
  free(s);
  
  END_CRITICAL;
}
