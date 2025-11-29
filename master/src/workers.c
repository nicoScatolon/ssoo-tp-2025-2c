#include "workers.h"
void enviarQueryAWorker(worker* workerElegido,char* path,int PC,int queryID){

    pthread_mutex_lock(&workerElegido->mutex);
    workerElegido->pathActual = strdup(path);
    workerElegido->idActual = queryID;
    pthread_mutex_unlock(&workerElegido->mutex);

    t_paquete*paquete = crearPaquete();
    agregarStringAPaquete(paquete,path);
    agregarIntAPaquete(paquete,queryID);
    agregarIntAPaquete(paquete,PC);
    enviarOpcode(NUEVA_QUERY,workerElegido->socket);
    enviarPaquete(paquete,workerElegido->socket);
    log_debug(logger,"QueryId a enviar <%d>",queryID);
    eliminarPaquete(paquete);
    sem_post(&sem_execute);
}
worker * obtenerWorkerLibre(){
    pthread_mutex_lock(&listaWorkers.mutex);
    for (int i = 0; i <list_size(listaWorkers.lista); i++)
    {
        worker * workerLibre = list_get(listaWorkers.lista,i);
        pthread_mutex_lock(&workerLibre->mutex);
        if(!workerLibre->ocupado){
            workerLibre->ocupado = true;
            pthread_mutex_unlock(&workerLibre->mutex);
            pthread_mutex_unlock(&listaWorkers.mutex);
            return workerLibre;
        }
        pthread_mutex_unlock(&workerLibre->mutex);
    }
    pthread_mutex_unlock(&listaWorkers.mutex);
    return NULL;
}
bool hayWorkerLibre(){
    bool hay = false;
    pthread_mutex_lock(&listaWorkers.mutex);
    for (int i = 0; i < list_size(listaWorkers.lista); i++) {
        worker* w = list_get(listaWorkers.lista, i);
        pthread_mutex_lock(&w->mutex);
        if (!w->ocupado) { hay = true; pthread_mutex_unlock(&w->mutex); break; }
        pthread_mutex_unlock(&w->mutex);
    }
    pthread_mutex_unlock(&listaWorkers.mutex);
    return hay;
}
worker* buscarWorkerPorQueryId(int idQuery){
    pthread_mutex_lock(&listaWorkers.mutex);
    for (int i = 0; i < list_size(listaWorkers.lista); i++)
    {
        worker * workerA = list_get(listaWorkers.lista,i);
        if (workerA->idActual == idQuery)
        {
            pthread_mutex_unlock(&listaWorkers.mutex);
            return workerA;
        }
        
    }
    pthread_mutex_unlock(&listaWorkers.mutex);
    return NULL;
}

void liberarWorker(worker* w){
    pthread_mutex_lock(&w->mutex);
    w->ocupado = false;
    w->idActual = -1;
    w->pathActual = NULL; 
    pthread_mutex_unlock(&w->mutex);
    sem_post(&sem_workers_libres);
    // if(strcmp(configM->algoritmoPlanificacion,"PRIORIDADES") == 0){
    //     sem_post(&sem_ready);
    // }
}

int obtenerPosicionWPorId(int idBuscado) {
    pthread_mutex_lock(&listaWorkers.mutex);
    for (int i = 0; i < list_size(listaWorkers.lista); i++) {
        worker* elem = (worker*) list_get(listaWorkers.lista, i);
        if (elem->workerID == idBuscado) {   // <-- antes estaba idActual
            pthread_mutex_unlock(&listaWorkers.mutex);
            return i;
        }
    }
    pthread_mutex_unlock(&listaWorkers.mutex);
    return -1;
}
worker* buscarWorkerPorSocket(int socket){
    pthread_mutex_lock(&listaWorkers.mutex);
    for (int i = 0; i < list_size(listaWorkers.lista); i++) {
        worker* w = list_get(listaWorkers.lista, i);
        if (w->socket == socket) {
            pthread_mutex_unlock(&listaWorkers.mutex);
            return w;
        }
    }
    pthread_mutex_unlock(&listaWorkers.mutex);
    return NULL;
}
void eliminarWorker(worker* workerEliminar){
    int posicion = obtenerPosicionWPorId(workerEliminar->workerID);

    pthread_mutex_lock(&listaWorkers.mutex);
    list_remove(listaWorkers.lista,posicion);
    pthread_mutex_unlock(&listaWorkers.mutex);

    pthread_mutex_destroy(&workerEliminar->mutex);
    free(workerEliminar->pathActual);
    free(workerEliminar);
}
worker* buscarWorkerPorId(int idBuscado) {
    for (int i = 0; i < list_size(listaWorkers.lista); i++) {
        worker* w = list_get(listaWorkers.lista, i);
        if (w->workerID == idBuscado) {
            return w;
        }
    }
    return NULL; // No se encontró
}