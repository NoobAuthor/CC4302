#define _DEFAULT_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <nthread.h>
#include <stdarg.h>

#include "subasta.h"

#pragma GCC diagnostic ignored "-Wunused-function"

static int esperar(pthread_t thr);

// ==================================================
// Variables globales: el timeout default

static int default_timeout= -1; // en milisegundos

// ==================================================
// Funcion para que un thread haga una pausa

static void msleep(int delay) {
  usleep(delay*1000);
}

// ==================================================
// Funcion para reportar un error y detener la ejecucion

void fatalError( char *procname, char *format, ... ) {
  va_list ap;

  fprintf(stderr,"Error Fatal en la rutina %s y la tarea \n", procname);
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);

  exit(1); /* shutdown */
}

// ==================================================
// Funciones para reportar nombre de los threads

__thread char *threadName= NULL;

static void setThreadName(char *name) {
  threadName= name;
}

static char *getThreadName(void) {
  return threadName;
}

// print_msg:
//   1   => con mensajes y delay
//   0   => sin mensajes, con delay
//   -1  => sin mensajes, sin delay

typedef struct {
  nSubasta s;
  int print_msg;
  char *nom;
  int oferta, timeout, ret;
} Param;

// ==================================================
// Funciones para los tests con timeout que
// se vencen o que no se vencen

static void *agente(void *ptr) {
  Param *pparm= ptr;
  nSubasta s= pparm->s;
  int print_msg= pparm->print_msg;
  char *nom= pparm->nom;
  int oferta= pparm->oferta;
  int timeout= pparm->timeout;
  setThreadName(nom);
  if (print_msg>0)
    printf("%s ofrece %d timeout=%d\n", nom, oferta, timeout);
  int ret= nOfrecer(s, oferta, timeout*1000);
  if (print_msg>0) {
    if (ret)
      printf("%s se adjudico un item a %d\n", nom, oferta);
    else
      printf("%s fallo con su oferta de %d\n", nom, oferta);
  }
  pparm->ret= ret;
  return pparm;
}

static pthread_t crearAgente(nSubasta s, char *nom, int oferta, int timeout)
{
  Param parm= { s, 1, nom, oferta, timeout };
  Param *pparm= malloc(sizeof(Param));
  *pparm= parm;
  pthread_t thr;
  if (pthread_create(&thr, NULL, agente, pparm)!=0) {
    perror("nom");
    exit(1);
  }
  return thr;
}

static int test_timeouts1() {
  printf("2 Timeouts: el de pedro no se vence, el de juan si\n");
  nSubasta s= nNuevaSubasta(2);
  pthread_t pedro= crearAgente(s, "pedro", 2, 3);
  pthread_t juan= crearAgente(s, "juan", 3, 1);
  msleep(2000);
  int u;
  int recaud= nAdjudicar(s, &u);
  if (!esperar(pedro))
    fatalError("test", "p debio ganar con 2\n");
  if (esperar(juan))
    fatalError("test", "p debio perder con 3 por el timeout\n");
  if (recaud!=2)
    fatalError("test", "La recaudacion debio ser 2 y no %d\n", recaud);
  if (u!=1)
    fatalError("test", "Quedaron %d unidades sin vender. Debio quedar 1.\n",
               u);
  printf("El monto recaudado es %d y quedaron %d unidades sin vender\n",
         recaud, u);
  nDestruirSubasta(s);
  return 0;
}

#if 1
static int test_timeouts2() {
  nSubasta s= nNuevaSubasta(2);
  pthread_t pedro= crearAgente(s, "pedro", 1, 4);
  pthread_t juan= crearAgente(s, "juan", 3, 3);
  pthread_t diego= crearAgente(s, "diego", 4, 2);
  pthread_t pepe= crearAgente(s, "pepe", 2, 1);
  if (esperar(diego))
    fatalError("test", "diego debio perder con 1\n");
  if (esperar(pepe))
    fatalError("test", "pepe debio perder con 2\n");
  int u;
  int recaud= nAdjudicar(s, &u);
  if (recaud!=4)
    fatalError("test", "La recaudacion debio ser 4 y no %d\n", recaud);
  if (u!=0)
    fatalError("test", "Quedaron %d unidades sin vender\n", u);
  if (!esperar(pedro))
    fatalError("test", "juan debio ganar con 3\n");
  if (!esperar(juan))
    fatalError("test", "diego debio ganar con 4\n");
  printf("El monto recaudado es %d y quedaron %d unidades sin vender\n",
         recaud, u);
  nDestruirSubasta(s);
  return 0;
}
#endif

// ==================================================
// Funciones para los tests originales de la tarea 4

static void *aleatorio(void *ptr) {
  Param *pparm= ptr;
  nSubasta s= pparm->s;
  int print_msg= pparm->print_msg;
  char *nom= pparm->nom;
  int oferta= pparm->oferta;
  if (print_msg>=0) {
    setThreadName(nom);
    int delay= random()%1000;
    msleep(delay);
  }
  if (print_msg>0)
    printf("%s ofrece %d\n", nom, oferta);
  int ret= nOfrecer(s, oferta, default_timeout);
  if (print_msg>0) {
    if (ret)
      printf("%s se adjudico un item a %d\n", nom, oferta);
    else
      printf("%s fallo con su oferta de %d\n", nom, oferta);
  }
  pparm->ret= ret;
  return pparm;
}

static pthread_t crearAleatorio(nSubasta s, int print_msg, char *nom, int oferta)
{
  Param parm= { s, print_msg, nom, oferta };
  Param *pparm= malloc(sizeof(Param));
  *pparm= parm;
  pthread_t thr;
  if (pthread_create(&thr, NULL, aleatorio, pparm)!=0) {
    perror("nom");
    exit(1);
  }
  return thr;
}

static int esperar(pthread_t thr) {
  void *pparm;
  pthread_join(thr, &pparm);
  int ret= ((Param*)pparm)->ret;
  free(pparm);
  return ret;
}

static void *oferente(void *ptr) {
  Param *pparm= ptr;
  nSubasta s= pparm->s;
  int print_msg= pparm->print_msg;
  char *nom= pparm->nom;
  int oferta= pparm->oferta;
  setThreadName(nom);
  if (print_msg>0)
    printf("%s ofrece %d\n", nom, oferta);
  int ret= nOfrecer(s, oferta, default_timeout);
  if (print_msg>0) {
    if (ret)
      printf("%s se adjudico un item a %d\n", nom, oferta);
    else
      printf("%s fallo con su oferta de %d\n", nom, oferta);
  }
  pparm->ret= ret;
  return pparm;
}

static pthread_t crearOferente(nSubasta s, int print_msg, char *nom, int oferta)
{
  Param parm= { s, print_msg, nom, oferta };
  Param *pparm= malloc(sizeof(Param));
  *pparm= parm;
  pthread_t thr;
  if (pthread_create(&thr, NULL, oferente, pparm)!=0) {
    perror("nom");
    exit(1);
  }
  return thr;
}

typedef struct {
  int (*testFun)(int print_msg);
  int print_msg;
} ParamTest;

static void *genTest(void *ptr) {
  ParamTest *pparm= ptr;
  (*pparm->testFun)(pparm->print_msg);
  free(pparm);
  return NULL;
}

static pthread_t crearTest(int (*testFun)(int), int print_msg) {
  ParamTest parm= {testFun, print_msg};
  ParamTest *pparm= malloc(sizeof(ParamTest));
  *pparm= parm;
  pthread_t thr;
  if (pthread_create(&thr, NULL, genTest, pparm)!=0) {
    perror("pthread_create");
    exit(1);
  }
  return thr;
}

static int test1(int print_msg) {
  nSubasta s= nNuevaSubasta(2);
  pthread_t pedro= crearAleatorio(s, print_msg, "pedro", 1);
  pthread_t juan= crearAleatorio(s, print_msg, "juan", 3);
  pthread_t diego= crearAleatorio(s, print_msg, "diego", 4);
  pthread_t pepe= crearAleatorio(s, print_msg, "pepe", 2);
  if (esperar(pedro))
    fatalError("test", "pedro debio perder con 1\n");
  if (esperar(pepe))
    fatalError("test", "pepe debio perder con 2\n");
  int u;
  int recaud= nAdjudicar(s, &u);
  if (recaud!=7)
    fatalError("test", "La recaudacion debio ser 7 y no %d\n", recaud);
  if (u!=0)
    fatalError("test", "Quedaron %d unidades sin vender\n", u);
  if (!esperar(juan))
    fatalError("test", "juan debio ganar con 3\n");
  if (!esperar(diego))
    fatalError("test", "diego debio ganar con 4\n");
  if (print_msg>0)
    printf("El monto recaudado es %d y quedaron %d unidades sin vender\n",
            recaud, u);
  nDestruirSubasta(s);
  return 0;
}

static int test2(int print_msg) {
  nSubasta s= nNuevaSubasta(3);
  pthread_t ana= crearOferente(s, print_msg, "ana", 7);
  if (print_msg>=0)
    msleep(1000);
  pthread_t maria= crearOferente(s, print_msg, "maria", 3);
  if (print_msg>=0)
    msleep(1000);
  pthread_t ximena= crearOferente(s, print_msg, "ximena", 4);
  if (print_msg>=0)
    msleep(1000);
  pthread_t erika= crearOferente(s, print_msg, "erika", 5);
  if (print_msg>=0)
    msleep(1000);
  if (esperar(maria))
    fatalError("test", "maria debio perder con 3\n");
  pthread_t sonia= crearOferente(s, print_msg, "sonia", 6);
  if (print_msg>=0)
    msleep(1000);
  if (esperar(ximena))
    fatalError("test", "ximena debio perder con 4\n");
  int u;
  int recaud= nAdjudicar(s, &u);
  if (recaud!=18)
    fatalError("test", "La recaudacion debio ser 7 y no %d\n", recaud);
  if (u!=0)
    fatalError("test", "Quedaron %d unidades sin vender\n", u);
  if (!esperar(ana))
    fatalError("test", "ana debio ganar con 7\n");
  if (!esperar(erika))
    fatalError("test", "erika debio ganar con 5\n");
  if (!esperar(sonia))
    fatalError("test", "sonia debio ganar con 6\n");
  if (print_msg>0)
    printf("El monto recaudado es %d y quedaron %d unidades sin vender\n",
            recaud, u);
  nDestruirSubasta(s);
  return 0;
}

static int test3(int print_msg) {
  nSubasta s= nNuevaSubasta(5);
  pthread_t tomas= crearOferente(s, print_msg, "tomas", 2);
  if (print_msg>=0)
    msleep(1000);
  pthread_t monica= crearOferente(s, print_msg, "monica", 3);
  if (print_msg>=0)
    msleep(1000);
  int u;
  int recaud= nAdjudicar(s, &u);
  if (recaud!=5)
    fatalError("test", "La recaudacion debio ser 5 y no %d\n", recaud);
  if (u!=3)
    fatalError("test", "Quedaron %d unidades sin vender\n", u);
  if (print_msg>0)
    printf("El monto recaudado es %d y quedaron %d unidades sin vender\n",
            recaud, u);
  if (!esperar(tomas))
    fatalError("test", "tomas debio ganar con 2\n");
  if (!esperar(monica))
    fatalError("test", "monica debio ganar con 3\n");
  nDestruirSubasta(s);
  return 0;
}

// #define N 30
// 
// #ifndef OPT
// #define M 150
// #else
// #define M 5000
// #endif

#define N 30

#ifndef OPT
#define M 50
#else
#define M 250
#endif

void suite(int timeout) {
  default_timeout= timeout;
        
#if 1
  printf("una sola subasta con tiempos aleatorios\n");
  test1(1);
  printf("test aprobado\n");
  printf("-------------\n");
#endif

  printf("una sola subasta con tiempos deterministicos\n");
  test2(1);
  printf("test aprobado\n");
  printf("-------------\n");

  printf("una sola subasta con menos oferentes que unidades disponibles\n");
  test3(1);
  printf("test aprobado\n");
  printf("-------------\n");

  // nSetTimeSlice(1);
  printf("Test de robustez\n");
  printf("%d subastas en paralelo\n", N);
  pthread_t *tasks1= malloc(M*sizeof(pthread_t));
  pthread_t *tasks2= malloc(M*sizeof(pthread_t));
  pthread_t *tasks3= malloc(M*sizeof(pthread_t));
  for (int k=1; k<N; k++) {
    tasks1[k]= crearTest(test1, 0);
    tasks2[k]= crearTest(test2, 0);
    tasks3[k]= crearTest(test3, 0);
  }
  tasks1[0]= crearTest(test1, 1);
  tasks2[0]= crearTest(test2, 1);
  tasks3[0]= crearTest(test3, 1);
  for (int k=0; k<N; k++) {
    pthread_join(tasks1[k], NULL);
    pthread_join(tasks2[k], NULL);
    pthread_join(tasks3[k], NULL);
  }
  printf("test aprobado\n");
  printf("-------------\n");
  printf("%d subastas en paralelo\n", M*2);
  for (int k=1; k<M; k++) {
    tasks1[k]= crearTest(test1, -1);
    tasks2[k]= crearTest(test2, -1);
  }
  tasks1[0]= crearTest(test1, 1);
  tasks2[0]= crearTest(test2, 1);
  pthread_join(tasks1[0], NULL);
  pthread_join(tasks2[0], NULL);
  printf("Enterrando tareas.  Cada '.' son 30 tareas enterradas.\n");
  for (int k=1; k<M; k++) {
    pthread_join(tasks1[k], NULL);
    pthread_join(tasks2[k], NULL);
    if (k%10==0) printf(".");
  }
  free(tasks1);
  free(tasks2);
  free(tasks3);
  printf("\ntest aprobado\n");
  printf(  "-------------\n");
}

int main(int argc, char *argv[]) {
  test_timeouts1();
#if 1
  printf("=====================================\n");
  printf("Tests con timeouts que no se vencen\n");
  printf("=====================================\n\n");
  suite(1000*5*60); // 5 minutos de timeout
#endif
  printf("=====================================\n");
  printf("Tests de compatibilidad: sin timeouts\n");
  printf("=====================================\n\n");
  suite(-1);         // Sin timeout
  printf("Felicitaciones: paso todos los tests\n");
  return 0;
}
