#include "planificadores.h"


// void planificarConFIFO(){
//     // Primero deberiamos tener en cuenta de que haya al menos un query_Control conectado, 
//     // Osea cuando se conecta una queryControl deberiamos tener pthread_cond_unlock
//     //Luego deberiamos ver si hay un worker disponible
//     // En caso afirmativo mandamos a ejecutar esa query al primer worker que encontremos
//     // En caso negativo la query deberia esperar en la lista
// }

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

void* planificadorFIFO() {

    while(1) {

        pthread_mutex_lock(&mutex_cv_planif);
        while (!hay_trabajo_para_planificar()) {
            pthread_cond_wait(&cv_planif, &mutex_cv_planif);
        }
        pthread_mutex_unlock(&mutex_cv_planif);
        
        worker* workerElegido = obtenerWorkerLibre();
        if (workerElegido == NULL) {
            log_warning(logger,"El worker es nulo");
            continue;
        }

        query* queryElegida = obtenerQuery();
        if (queryElegida == NULL) {
            log_warning(logger,"La query es nula");
            continue;
        }

        cambioEstado(&listaExecute,queryElegida);
        enviarQueryAWorker(workerElegido, queryElegida->path,queryElegida->qcb->PC,queryElegida->qcb->queryID);

    }
}

query* obtenerQuery(){
    return (query*) listRemove(&listaReady);
}

void cambioEstado(t_list_mutex* lista, query* elemento){
    listaAdd(elemento,lista);
}

// void* enviarQueryAWorker(worker*)

void despertar_planificador(){
    pthread_mutex_lock(&mutex_cv_planif);
    pthread_cond_signal(&cv_planif);
    pthread_mutex_unlock(&mutex_cv_planif);
}

bool hay_trabajo_para_planificar(){
    bool ready_no_vacia = !esListaVacia(&listaReady);
    bool hay_worker_libre = false;

    pthread_mutex_lock(&listaWorkers.mutex);
    for (int i = 0; i < list_size(listaWorkers.lista); i++) {
        worker* w = list_get(listaWorkers.lista, i);
        pthread_mutex_lock(&w->mutex);
        if (!w->ocupado) {
            hay_worker_libre = true;
            pthread_mutex_unlock(&w->mutex);
            break;
        }
        pthread_mutex_unlock(&w->mutex);
    }
    pthread_mutex_unlock(&listaWorkers.mutex);

    return ready_no_vacia && hay_worker_libre;
}
void enviarQueryAWorker(worker* workerElegido,char* path,int PC,int queryID){
    workerElegido->pathActual = path;
    workerElegido->idActual = queryID;

    t_paquete*paquete = crearPaquete();
    agregarStringAPaquete(paquete,path);
    agregarIntAPaquete(paquete,PC);
    agregarIntAPaquete(paquete,queryID);
    enviarOpcode(NUEVA_QUERY,workerElegido->socket);
    enviarPaquete(paquete,workerElegido->socket);

    eliminarPaquete(paquete);
}

queryControl* buscarQueryControlPorId(int idQuery){
    pthread_mutex_lock(&listaQueriesControl.mutex);
    for (int i = 0; i < list_size(listaQueriesControl.lista); i++)
    {
        queryControl * queryC = list_get(listaQueriesControl.lista,i);
        if (queryC->queryControlID == idQuery)
        {
            pthread_mutex_unlock(&listaQueriesControl.mutex);
            return queryC;
        }
        
    }
    pthread_mutex_unlock(&listaQueriesControl.mutex);
    return NULL;
}
worker* buscarWorkerPorId(int idQuery){
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
    pthread_mutex_unlock(&listaQueriesControl.mutex);
    return NULL;
}


int obtenerPosicionQCPorId(int idBuscado) {
    pthread_mutex_lock(&listaQueriesControl.mutex);
    for (int i = 0; i < list_size(listaQueriesControl.lista); i++) {
        queryControl* elem = (queryControl*) list_get(listaQueriesControl.lista, i);
        if (elem->queryControlID == idBuscado) {
            pthread_mutex_unlock(&listaQueriesControl.mutex);
            return i;
        }
    }
    pthread_mutex_unlock(&listaQueriesControl.mutex);
    return -1; 
}


int obtenerPosicionWPorId(int idBuscado) {
    pthread_mutex_lock(&listaWorkers.mutex);
    for (int i = 0; i < list_size(listaWorkers.lista); i++) {
        worker* elem = (worker*) list_get(listaWorkers.lista, i);
        if (elem->idActual == idBuscado) {
            pthread_mutex_unlock(&listaWorkers.mutex);
            return i;
        }
    }
    pthread_mutex_unlock(&listaWorkers.mutex);
    return -1; 
}

void liberarWorker(worker* workerA){
    workerA->ocupado= false;
    workerA->idActual=-1;
    workerA->pathActual = "";
}

void* Aging(void* arg) {
    while (1) {
        usleep(configM->tiempoAging * 1000); // si está en ms

        pthread_mutex_lock(&listaReady.mutex);
        for (int i = 0; i < list_size(listaReady.lista); i++) {
            query* q = list_get(listaReady.lista, i);
            if (q->qcb->prioridad > 0) {
                q->qcb->prioridad -= 1;
            }
        }

        pthread_mutex_unlock(&listaReady.mutex);
    }
    return NULL;
}
void* planificadorPrioridad() {

    while(1) {

        pthread_mutex_lock(&mutex_cv_planif);
        while (!hay_trabajo_para_planificar()) {
            pthread_cond_wait(&cv_planif, &mutex_cv_planif);
        }
        pthread_mutex_unlock(&mutex_cv_planif);
        if (!hayWorkerLibre())
        {
            //Deberia despertar el hilo que evalua el desalojo
            
        }
        
        worker* workerElegido = obtenerWorkerLibre();
        if (workerElegido == NULL) {
            log_warning(logger,"El worker es nulo");
            continue;
        }

        query* queryElegida = obtenerQueryDeMenorPrioridad();
        if (queryElegida == NULL) {
            log_warning(logger,"La query es nula");
            continue;
        }

        cambioEstado(&listaExecute,queryElegida);
        enviarQueryAWorker(workerElegido, queryElegida->path,queryElegida->qcb->PC,queryElegida->qcb->queryID);
    }
}

query* obtenerQueryDeMenorPrioridad() {

    if (esListaVacia(&listaReady)) {;
        return NULL; 
    }
    pthread_mutex_lock(&listaReady.mutex);

    int indiceMejor = 0;
    query* mejor = list_get(listaReady.lista, 0);


    for (int i = 1; i < list_size(listaReady.lista); i++) {
        query* q = list_get(listaReady.lista, i);
        if (q->qcb->prioridad < mejor->qcb->prioridad) {
            mejor = q;
            indiceMejor = i;
        }
    }

    query* seleccionada = list_remove(listaReady.lista, indiceMejor);
    pthread_mutex_unlock(&listaReady.mutex);
    return seleccionada; 
}

bool hayWorkerLibre(){
    pthread_mutex_lock(&listaWorkers.mutex);
    for (int i = 0; i < list_size(listaWorkers.lista); i++)
    {
        worker* w = list_get(listaWorkers.lista,i);
        if (!w->ocupado)
        {
            pthread_mutex_unlock(&listaWorkers.mutex);
            return true;
        }
    }
    pthread_mutex_unlock(&listaWorkers.mutex);
    return false;
}