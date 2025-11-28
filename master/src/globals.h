#ifndef GLOBALS_H
#define GLOBALS_H

#include "utils/config.h"
#include "utils/logs.h"
#include "utils/listas.h"
#include <commons/collections/list.h>
#include "semaphore.h"
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>


// ✅ SOLO DECLARACIONES con extern - NO definiciones
extern pthread_mutex_t mutex_id_queryControl;
extern pthread_mutex_t mutex_id_worker;
extern pthread_mutex_t mutex_grado;
extern pthread_mutex_t mutex_cantidadQueriesControl;
extern pthread_mutex_t mutex_cv_planif;
extern pthread_cond_t  cv_planif;
// extern pthread_mutex_t mutex_lista_workers;
extern uint32_t siguienteIdQueryControl;
extern uint32_t siguienteIdWorker;
extern uint32_t gradoMultiprogramacion;
extern uint32_t cantidadQueriesControl;

extern t_log* logger;
extern configMaster* configM;

extern sem_t sem_ready;
extern sem_t sem_desalojo;
extern sem_t sem_workers_libres;
extern sem_t sem_execute;

extern t_list_mutex listaReady;
extern t_list_mutex listaExecute;
extern t_list_mutex listaWorkers;
extern t_list_mutex listaQueriesControl;

typedef enum {
    QUERY_READY,
    QUERY_EXECUTE,
    QUERY_EXIT
} estadoQuery;

typedef struct {
    char* path;
    int socket;
    int prioridad;
    int queryControlID;
    pthread_mutex_t mutex;
} queryControl;

typedef struct {
    char* pathActual;
    int socket;
    int socketDesalojo;
    int workerID;
    int idActual;
    bool ocupado;
    pthread_mutex_t mutex;
} worker;

typedef struct {
    int PC;
    int prioridad;
    int queryID;
} qcb_t;

typedef struct {
    char* path;
    qcb_t* qcb;
    pthread_mutex_t mutex;
} query;

#endif