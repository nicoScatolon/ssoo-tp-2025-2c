#include "globals.h"

#include "globals.h"


t_list* READY = NULL;
t_list* EXECUTE = NULL;
t_list* EXIT = NULL;
t_list* workers = NULL;
t_list* queriesControl = NULL;

uint32_t siguienteIdQueryControl = 0;
uint32_t siguienteIdWorker = 0;
uint32_t gradoMultiprogramacion = 0;
uint32_t cantidadQueriesControl = 0;

pthread_mutex_t mutex_id_queryControl = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_id_worker = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_grado = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cantidadQueriesControl = PTHREAD_MUTEX_INITIALIZER;