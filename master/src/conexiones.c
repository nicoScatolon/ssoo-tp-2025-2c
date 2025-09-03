#include "utils/config.h"
#include "utils/logs.h"
#include "conexiones.h"



void agregarQueryControl(char* path,int socketCliente, int prioridad){
    queryControl* nuevaQuery = malloc(sizeof(queryControl));
    nuevaQuery->path = strdup(path);
    nuevaQuery->socket = socketCliente;
    nuevaQuery->ocupado = FALSE;
    nuevaQuery->prioridad = prioridad;
    nuevaQuery->queryID = generarIdQueryControl();
    pthread_mutex_init(&nuevaQuery->mutex,NULL);
    list_add(ready,nuevaQuery);
    pthread_mutex_lock(&mutex_cantidadQueries);
    cantidadQueries++;
    pthread_mutex_unlock(&mutex_cantidadQueries);
    log_info(log, "## Se conecta un Query Control para ejecutar la Query <PATH_QUERY> <%s> con prioridad <PRIORIDAD> <%d> - Id asignado: <QUERY_ID> <%d> . Nivel multiprogramación <CANTIDAD> <%d>",path,prioridad,nuevaQuery->queryID,cantidadQueries);
}
void agregarWorker(int socketCliente){
    worker * nuevoWorker = malloc(sizeof(worker));
    nuevoWorker->pathActual = strdup("");
    nuevoWorker->socket = socketCliente;
    nuevoWorker->ocupado = FALSE;
    nuevoWorker->workerID = generarIdWorker();
    pthread_mutex_init(&nuevoWorker->mutex,NULL);
    list_add(workers,nuevoWorker);
    pthread_mutex_lock(&mutex_grado);
    gradoMultiprogramacion++;
    pthread_mutex_unlock(&mutex_grado);
    log_info(log,"Conexión de Worker: ## Se conecta el Worker <WORKER_ID> <%d> - Cantidad total de Workers: <CANTIDAD> <%d> ",nuevoWorker->workerID,gradoMultiprogramacion);

}
uint32_t generarIdQueryControl() {
    pthread_mutex_lock(&mutex_id_query);
    uint32_t id = siguienteIdQuery++;
    pthread_mutex_unlock(&mutex_id_query);
    return id;
}
uint32_t generarIdWorker() {
    pthread_mutex_lock(&mutex_id_worker);
    uint32_t id = siguienteIdWorker++;
    pthread_mutex_unlock(&mutex_id_worker);
    return id;
}