#include "globals.h"



t_list_mutex listaReady;
t_list_mutex listaExecute;
t_list_mutex listaWorkers;
t_list_mutex listaQueriesControl;


uint32_t siguienteIdQueryControl = 0;
uint32_t siguienteIdWorker = 0;
uint32_t gradoMultiprogramacion = 0;
uint32_t cantidadQueriesControl = 0;

pthread_mutex_t mutex_id_queryControl = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_id_worker = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_grado = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cantidadQueriesControl = PTHREAD_MUTEX_INITIALIZER;


sem_t sem_desalojo;
sem_t sem_workers_libres; 
sem_t sem_ready;


