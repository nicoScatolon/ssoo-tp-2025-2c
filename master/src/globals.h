#ifndef GLOBALS_H
#define GLOBALS_H

#include "utils/config.h"
#include "utils/logs.h"
#include <commons/collections/list.h>
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>

// ✅ SOLO DECLARACIONES con extern - NO definiciones
extern pthread_mutex_t mutex_id_queryControl;
extern pthread_mutex_t mutex_id_worker;
extern pthread_mutex_t mutex_grado;
extern pthread_mutex_t mutex_cantidadQueriesControl;

extern uint32_t siguienteIdQueryControl;
extern uint32_t siguienteIdWorker;
extern uint32_t gradoMultiprogramacion;
extern uint32_t cantidadQueriesControl;

extern t_log* logger;
extern configMaster* configM;

extern t_list* READY;
extern t_list* EXECUTE;
extern t_list* EXIT;
extern t_list* workers;
extern t_list* queriesControl;

// ✅ Enums y structs (estos SÍ van en .h)
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
    int workerID;
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