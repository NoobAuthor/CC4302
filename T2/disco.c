#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "disco.h"

static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t c = PTHREAD_COND_INITIALIZER;

static int next_dama_ticket = 0;
static int next_varon_ticket = 0;

static int serving_dama = 0;
static int serving_varon = 0;

static char *current_dama_name = NULL;
static char *current_varon_name = NULL;

void discoInit(void) {
}

void discoDestroy(void) {
    pthread_mutex_destroy(&m);
    pthread_cond_destroy(&c);
}

char *dama(char *nom) {
    char *partner_name;
    
    pthread_mutex_lock(&m);
    
    int my_ticket = next_dama_ticket++;
    
    while (my_ticket != serving_dama || current_varon_name == NULL) {
        pthread_cond_wait(&c, &m);
    }
    
    current_dama_name = nom;
    partner_name = current_varon_name;
    
    current_varon_name = NULL;
    
    serving_dama++;
    
    pthread_cond_broadcast(&c);
    
    pthread_mutex_unlock(&m);
    return partner_name;
}

char *varon(char *nom) {
    char *partner_name;
    
    pthread_mutex_lock(&m);
    
    int my_ticket = next_varon_ticket++;
    
    while (my_ticket != serving_varon) {
        pthread_cond_wait(&c, &m);
    }
    
    current_varon_name = nom;
    
    pthread_cond_broadcast(&c);
    
    while (current_varon_name != NULL) {
        pthread_cond_wait(&c, &m);
    }
    
    partner_name = current_dama_name;
    
    serving_varon++;
    
    pthread_cond_broadcast(&c);
    
    pthread_mutex_unlock(&m);
    return partner_name;
}
