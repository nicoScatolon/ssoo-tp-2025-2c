#include "planificadores.h"

// ------------------- Planificadores -------------------
void* planificadorFIFO() {

    while(1) {

        sem_wait(&sem_ready);
        sem_wait(&sem_workers_libres);
        
        worker* workerElegido = obtenerWorkerLibre();
        if (workerElegido == NULL) {  // Caso posible, si el worker es null puede ser porq cortò su ejecuciòn, no es error.
            log_warning(logger, "El worker se desconecto, se replanifica.");
            continue;
        }

        query* queryElegida = obtenerQuery();
        if (queryElegida == NULL) { // Caso posible, si la query es null puede ser porq el query control cortò su ejecuciòn, no es error.
            log_warning(logger,"La query se desconecto, se libera worker y se replanifica.");
            liberarWorker(workerElegido);
            continue;
        }

        cambioEstado(&listaExecute,queryElegida);
        log_info(logger, "## Se envía la Query <%d> (<%d>) al Worker <%d>", queryElegida->qcb->queryID,queryElegida->qcb->prioridad,workerElegido->workerID);
        enviarQueryAWorker(workerElegido, queryElegida->path,queryElegida->qcb->PC,queryElegida->qcb->queryID);

    }
    return NULL; 
}

void* planificadorPrioridad(){
    while (1)
    {
        sem_wait(&sem_ready);
        if (!hayWorkerLibre())
        {
            sem_post(&sem_desalojo);
             sem_wait(&sem_workers_libres);
             sem_post(&sem_ready);
            continue;
        }
        worker* workerElegido = obtenerWorkerLibre();
        if (workerElegido == NULL) { // Caso posible, si el worker es null puede ser porq cortò su ejecuciòn, no es error.
            log_warning(logger, "El worker se desconecto, se replanifica.");
            continue;
           }
        query* queryElegida = sacarQueryDeMenorPrioridad(); 
        if (queryElegida == NULL) { // Caso posible, si la query es null puede ser porq el query control cortò su ejecuciòn, no es error.
            log_warning(logger,"La query se desconecto, se libera worker y se replanifica.");
            liberarWorker(workerElegido);
            continue;
        }
        cambioEstado(&listaExecute, queryElegida);
        log_info(logger, "## Se envía la Query <%d> (<%d>) al Worker <%d>", queryElegida->qcb->queryID,queryElegida->qcb->prioridad,workerElegido->workerID);
        enviarQueryAWorker(workerElegido, queryElegida->path,queryElegida->qcb->PC, queryElegida->qcb->queryID);
    }
}



// ------------------- Desalojo -------------------
void* evaluarDesalojo(){
    while(1){
        sem_wait(&sem_execute);
        sem_wait(&sem_desalojo);    
        query* queryReady = obtenerQueryMayorPrioridad();
        query* queryExec = obtenerQueryMenorPrioridad();

        if (queryReady && queryExec){
            if (queryReady->qcb->prioridad < queryExec->qcb->prioridad){
                worker* w = buscarWorkerPorQueryId(queryExec->qcb->queryID);
                enviarOpcode(DESALOJO_QUERY_PLANIFICADOR,w->socketDesalojo);
                t_paquete*paquete=crearPaquete();
                agregarIntAPaquete(paquete,queryExec->qcb->queryID);
                enviarPaquete(paquete,w->socketDesalojo);
                eliminarPaquete(paquete);
                log_debug(logger,"Notificando desalojo de Query <%d>",queryExec->qcb->queryID);
            }
        }
    }
}

// ------------------- Aging -------------------
void* aging(void *arg){
    int idQuery = (intptr_t)arg;
    while(1){
        usleep(configM->tiempoAging*1000);
        if(estaEnListaPorId(&listaReady,idQuery)){
            query* q = obtenerQueryPorID(idQuery);
            int prioridadVieja = q->qcb->prioridad;
            pthread_mutex_lock(&q->mutex);
            q->qcb->prioridad-=1;
            pthread_mutex_unlock(&q->mutex);
            log_info(logger,"##<%d> Cambio de prioridad: <%d> - <%d>",q->qcb->queryID,prioridadVieja,q->qcb->prioridad);
            sem_post(&sem_desalojo);
        }
        else if(estaEnListaPorId(&listaExecute,idQuery)){
            log_debug(logger,"Query <%d> esta en EXEC no se reduce prioridad",idQuery);
        }
        else{
            log_debug(logger,"Query <%d> finalizo, matando hilo",idQuery);
            pthread_exit(NULL);
        }
    }
    return NULL;
}

// ------------------- Búsquedas utilitarias -------------------
bool estaEnListaPorId(t_list_mutex * lista, int id) {
    pthread_mutex_lock(&lista->mutex);
    for (int i = 0; i < list_size(lista->lista); i++) {
        query *elemento = list_get(lista->lista, i);
        if (elemento->qcb->queryID == id) {
            pthread_mutex_unlock(&lista->mutex);
            return true;
        }
    }
    pthread_mutex_unlock(&lista->mutex);
    return false;
}
query* sacarDePorId(t_list_mutex* l,int queryID) {
    pthread_mutex_lock(&l->mutex);
    for (int i = 0; i < list_size(l->lista); i++) {
        query* q = list_get(l->lista, i);
        if (q->qcb->queryID == queryID) {
            q = list_remove(l->lista, i);
            pthread_mutex_unlock(&l->mutex);
            return q;
        }
    }
    pthread_mutex_unlock(&l->mutex);
    return NULL;
}
query* obtenerQueryPorID(int idQuery) {
    pthread_mutex_lock(&listaReady.mutex);
    
    for (int i = 0; i < list_size(listaReady.lista); i++) {
        query* q = list_get(listaReady.lista, i);
        if (q->qcb->queryID == idQuery) {
            pthread_mutex_unlock(&listaReady.mutex);
            return q;
        }
    }
    pthread_mutex_unlock(&listaReady.mutex);
    return NULL;  
}
query* obtenerQuery(){
    return (query*) listRemove(&listaReady);
}

void cambioEstado(t_list_mutex* lista, query* elemento){
    listaAdd(elemento,lista);
}

query* sacarQueryDeMenorPrioridad() {
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
query* sacarDeExecutePorId(int queryID) {
    pthread_mutex_lock(&listaExecute.mutex);
    for (int i = 0; i < list_size(listaExecute.lista); i++) {
        query* q = list_get(listaExecute.lista, i);
        if (q->qcb->queryID == queryID) {
            list_remove(listaExecute.lista, i);
            pthread_mutex_unlock(&listaExecute.mutex);
            return q;
        }
    }
    pthread_mutex_unlock(&listaExecute.mutex);
    return NULL;
}
query* obtenerQueryMayorPrioridad() {
    query* queryMayorPrioridad = NULL;
    pthread_mutex_lock(&listaReady.mutex);
    if (!list_is_empty(listaReady.lista)) {
        queryMayorPrioridad = list_get(listaReady.lista, 0);
        for (int i = 1; i < list_size(listaReady.lista); i++) {
            query* queryActual = list_get(listaReady.lista, i);
            if (queryActual->qcb->prioridad < queryMayorPrioridad->qcb->prioridad) {
                queryMayorPrioridad = queryActual;
            }
        }
    }
    pthread_mutex_unlock(&listaReady.mutex);

    if (queryMayorPrioridad != NULL) {
        log_debug(logger, "Se obtuvo la query ID <%d> como la de mayor prioridad en READY.", queryMayorPrioridad->qcb->queryID);
    } else {
        log_warning(logger, "La lista de Ready está vacía, no se pudo obtener ninguna query.");
    }
    return queryMayorPrioridad;
}
query* obtenerQueryMenorPrioridad() {
    query* queryMenorPrioridad = NULL;
    pthread_mutex_lock(&listaExecute.mutex);
    if (!list_is_empty(listaExecute.lista)) {
        queryMenorPrioridad = list_get(listaExecute.lista, 0);
        for (int i = 1; i < list_size(listaExecute.lista); i++) {
            query* queryActual = list_get(listaExecute.lista, i);
            if (queryActual->qcb->prioridad > queryMenorPrioridad->qcb->prioridad) {
                queryMenorPrioridad = queryActual;
            }
        }
    }
    pthread_mutex_unlock(&listaExecute.mutex);

    if (queryMenorPrioridad != NULL) {
        log_debug(logger, "Se obtuvo la query ID <%d> como la de menor prioridad en EXECUTE.", queryMenorPrioridad->qcb->queryID);
    } else {
        log_warning(logger, "La lista de EXECUTE está vacía, no se pudo obtener ninguna query."); // Usé log_warning, ya que puede no ser un error crítico
    }
    return queryMenorPrioridad;
}