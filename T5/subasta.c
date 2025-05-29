#include "subasta.h"
#include "pss.h"
#include <nthread-impl.h>
#include <stdlib.h>

typedef struct {
  nThread th;       // Thread que hace la oferta
  double price;     // Precio ofrecido
  nSubasta subasta; // Referencia a la subasta (para timeout handler)
} Offer;

struct subasta {
  int n;            // Número de unidades disponibles
  int closed;       // Indica si la subasta está cerrada
  PriQueue *offers; // Cola de prioridad para las ofertas
};

static void timeout_handler(nThread th) {
  // Esta función se ejecuta en contexto de señal, debe ser simple
  Offer *offer = (Offer *)th->ptr;
  if (offer != NULL) {
    // Remover la oferta de la cola de prioridad usando priDel
    priDel(offer->subasta->offers, offer);

    // Marcar como perdedor y liberar
    th->retPtr = (void *)0;
    free(offer);
    th->ptr = NULL;
  }
}

nSubasta nNuevaSubasta(int n) {
  nSubasta s = (nSubasta)malloc(sizeof(struct subasta));
  s->n = n;
  s->closed = 0;
  s->offers = makePriQueue(); // Usar PriQueue de pss.h
  return s;
}

int nOfrecer(nSubasta s, double precio, int timeout) {
  START_CRITICAL;

  if (s->closed) {
    END_CRITICAL;
    return 0;
  }

  Offer *offer = (Offer *)malloc(sizeof(Offer));
  offer->th = nSelf();
  offer->price = precio;
  offer->subasta = s;

  // Guardar referencia en el descriptor del thread
  nSelf()->ptr = offer;

  if (priLength(s->offers) >= s->n) {
    Offer *lowest = (Offer *)priPeek(s->offers);
    if (lowest && precio > lowest->price) {
      Offer *removed = (Offer *)priGet(s->offers);
      nThread loserThread = removed->th;
      priPut(s->offers, offer, precio);

      loserThread->retPtr = (void *)0;
      if (loserThread->status == WAIT_SUBASTA_TIMEOUT) {
        nth_cancelThread(loserThread);
      }
      setReady(loserThread);

      // Suspender este thread con el estado apropiado
      if (timeout < 0) {
        suspend(WAIT_SUBASTA);
      } else {
        suspend(WAIT_SUBASTA_TIMEOUT);
        nth_programTimer((long long)timeout * 1000000LL, timeout_handler);
      }

      free(removed);
      schedule();

      END_CRITICAL;

      // Verificar si fue despertado por timeout
      if (timeout >= 0 && nSelf()->status == WAIT_SUBASTA_TIMEOUT) {
        nth_cancelThread(nSelf());
      }

      nSelf()->ptr = NULL;
      return (int)(long long)nSelf()->retPtr;
    } else {
      nSelf()->ptr = NULL;
      free(offer);
      END_CRITICAL;
      return 0;
    }
  }

  // Hay espacio para una nueva oferta
  priPut(s->offers, offer, precio);

  if (timeout < 0) {
    suspend(WAIT_SUBASTA);
  } else {
    suspend(WAIT_SUBASTA_TIMEOUT);
    nth_programTimer((long long)timeout * 1000000LL, timeout_handler);
  }

  schedule();

  END_CRITICAL;

  // Verificar si fue despertado por timeout
  if (timeout >= 0 && nSelf()->status == WAIT_SUBASTA_TIMEOUT) {
    nth_cancelThread(nSelf());
  }

  nSelf()->ptr = NULL;
  return (int)(long long)nSelf()->retPtr;
}

double nAdjudicar(nSubasta s, int *punidades) {
  START_CRITICAL;

  s->closed = 1;
  double total = 0;
  int sold = 0;

  // Extraer todas las ofertas de la cola de prioridad
  int numOffers = priLength(s->offers);
  Offer **allOffers = (Offer **)malloc(numOffers * sizeof(Offer *));

  // Extraer todas las ofertas usando priGet
  int i = 0;
  while (!emptyPriQueue(s->offers)) {
    allOffers[i] = (Offer *)priGet(s->offers);
    i++;
  }

  // Ordenar las ofertas de mayor a menor precio
  for (int i = 0; i < numOffers - 1; i++) {
    for (int j = 0; j < numOffers - i - 1; j++) {
      if (allOffers[j]->price < allOffers[j + 1]->price) {
        Offer *temp = allOffers[j];
        allOffers[j] = allOffers[j + 1];
        allOffers[j + 1] = temp;
      }
    }
  }

  // Procesar las ofertas en orden de mayor a menor
  for (int i = 0; i < numOffers; i++) {
    Offer *offer = allOffers[i];
    if (sold < s->n) {
      // Oferta ganadora
      total += offer->price;
      sold++;
      offer->th->retPtr = (void *)1;
    } else {
      // Oferta perdedora
      offer->th->retPtr = (void *)0;
    }

    // Cancelar timeout si lo tenía
    if (offer->th->status == WAIT_SUBASTA_TIMEOUT) {
      nth_cancelThread(offer->th);
    }

    setReady(offer->th);
    offer->th->ptr = NULL;
    free(offer);
  }

  free(allOffers);

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
  while (!emptyPriQueue(s->offers)) {
    Offer *offer = (Offer *)priGet(s->offers);
    free(offer);
  }

  destroyPriQueue(s->offers);
  free(s);

  END_CRITICAL;
}
