#ifndef GLOBABLES_H
#define GLOBALES_H

#include "utils/config.h"
#include "utils/logs.h"
static uint32_t siguienteIdQuery = 0;
static pthread_mutex_t mutex_id_query = PTHREAD_MUTEX_INITIALIZER;
static uint32_t siguienteIdWorker = 0;
static pthread_mutex_t mutex_id_worker = PTHREAD_MUTEX_INITIALIZER;
static uint32_t gradoMultiprogramacion = 0;
static pthread_mutex_t mutex_grado = PTHREAD_MUTEX_INITIALIZER;
static uint32_t cantidadQueries = 0;
static pthread_mutex_t mutex_cantidadQueries = PTHREAD_MUTEX_INITIALIZER;


t_log* log;
t_list* ready;
t_list* execute;
t_list* exit;
t_list* workers;

typedef enum{
    QUERY_READY,
    QUERY_EXECUTE,
    QUERY_EXIT
} estadoQuery;

typedef struct 
{
    char*path;
    int socket;
    int prioridad;
    int queryID;
    bool ocupado;
    pthread_mutex_t mutex;
} queryControl;

typedef struct 
{
    char*pathActual;
    int socket;
    int workerID;
    bool ocupado;
    pthread_mutex_t mutex;
} worker;

#endif
