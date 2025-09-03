#include "utils/config.h"
#include "utils/logs.h"
#include "conexiones.h"



void agregarQueryControl(char* path,int socketCliente, int prioridad){
    queryControl *nuevaQueryControl = malloc(sizeof(queryControl));
    nuevaQueryControl->path = strdup(path);
    nuevaQueryControl->socket = socketCliente;
    nuevaQueryControl->prioridad = prioridad;
    nuevaQueryControl->queryControlID = generarIdQueryControl(); //Podemos utilizar el path o el socket para distinguirlo
    pthread_mutex_init(&nuevaQueryControl->mutex,NULL);
    list_add(queriesControl,nuevaQueryControl);
    pthread_mutex_lock(&mutex_cantidadQueriesControl);
    cantidadQueriesControl++;
    pthread_mutex_unlock(&mutex_cantidadQueriesControl);
    log_info(log, "## Se conecta un Query Control para ejecutar la Query <PATH_QUERY> <%s> con prioridad <PRIORIDAD> <%d> - Id asignado: <QUERY_ID> <%d> . Nivel multiprogramación <CANTIDAD> <%d>",path,prioridad,nuevaQueryControl->queryControlID,cantidadQueriesControl);
    agregarQuery(path,prioridad);
}
void agregarWorker(int socketCliente){
    worker *nuevoWorker = malloc(sizeof(worker));
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
void agregarQuery(char* path,int prioridad){
    query *nuevaQuery = malloc(sizeof(query));
    nuevaQuery->path = strdup(path);
    qcb_t *nuevoqcb = malloc(sizeof(qcb_t));
    nuevaQuery->qcb=nuevoqcb;
    nuevaQuery->qcb->prioridad = prioridad;
    nuevaQuery->qcb->queryID = generarIdQuery();
    nuevaQuery->qcb->PC = 0;
    pthread_mutex_init(&nuevaQuery->mutex,NULL);
    list_add(ready,nuevaQuery);
    log_debug(log,"Se encolo query ID <%d> en READY",nuevaQuery->qcb->queryID);
}
uint32_t generarIdQueryControl() {
    pthread_mutex_lock(&mutex_id_queryControl);
    uint32_t id = siguienteIdQueryControl++;
    pthread_mutex_unlock(&mutex_id_queryControl);
    return id;
}
uint32_t generarIdWorker() {
    pthread_mutex_lock(&mutex_id_worker);
    uint32_t id = siguienteIdWorker++;
    pthread_mutex_unlock(&mutex_id_worker);
    return id;
}
uint32_t generarIdQuery() {
    pthread_mutex_lock(&mutex_id_query);
    uint32_t id = siguienteIdQuery++;
    pthread_mutex_unlock(&mutex_id_query);
    return id;
}