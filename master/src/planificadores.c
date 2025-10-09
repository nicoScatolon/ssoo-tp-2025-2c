#include "planificadores.h"

// ------------------- Predicados/Sync -------------------
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

bool hay_trabajo_para_planificar(void) {
    return !esListaVacia(&listaReady) && hayWorkerLibre();
}

void despertar_planificador(){
    pthread_mutex_lock(&mutex_cv_planif);
    pthread_cond_signal(&cv_planif);
    pthread_mutex_unlock(&mutex_cv_planif);
}

// ------------------- Helpers de colas/estados -------------------
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

query* obtenerQuery(){
    return (query*) listRemove(&listaReady);
}

void cambioEstado(t_list_mutex* lista, query* elemento){
    listaAdd(elemento,lista);
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

// ------------------- I/O con Worker -------------------
void enviarQueryAWorker(worker* workerElegido,char* path,int PC,int queryID){

    pthread_mutex_lock(&workerElegido->mutex);
    workerElegido->pathActual = path;
    workerElegido->idActual = queryID;
    pthread_mutex_unlock(&workerElegido->mutex);

    t_paquete*paquete = crearPaquete();
    agregarStringAPaquete(paquete,path);
    agregarIntAPaquete(paquete,PC);
    agregarIntAPaquete(paquete,queryID);
    enviarOpcode(NUEVA_QUERY,workerElegido->socket);
    enviarPaquete(paquete,workerElegido->socket);

    eliminarPaquete(paquete);

    log_info(logger, "## Se envía la Query %d al Worker %d", queryID, workerElegido->workerID);
}


// ------------------- Planificadores -------------------
void* planificadorFIFO() {

    while(1) {

        sem_wait(&sem_ready);
        sem_wait(&sem_workers_libres);
        
        worker* workerElegido = obtenerWorkerLibre();
        if (workerElegido == NULL) {
            log_warning(logger,"El worker es nulo");
            continue;
        }

        query* queryElegida = obtenerQuery();
        if (queryElegida == NULL) {
            log_warning(logger,"El worker es nulo");
            continue;
        }

        cambioEstado(&listaExecute,queryElegida);
        enviarQueryAWorker(workerElegido, queryElegida->path,queryElegida->qcb->PC,queryElegida->qcb->queryID);
    }
    return NULL; 
}

void* planificadorPrioridad() {
    while(1){
        pthread_mutex_lock(&mutex_cv_planif);
        while (!hay_trabajo_para_planificar()) {
            pthread_cond_wait(&cv_planif, &mutex_cv_planif);
        }
        pthread_mutex_unlock(&mutex_cv_planif);

        // Toma un worker libre (marca ocupado = true)
        worker* workerElegido = obtenerWorkerLibre();
        if (workerElegido == NULL) {
            log_warning(logger,"El worker es nulo");
            continue;
        }

        // Toma la READY de mayor prioridad (menor número)
        query* queryElegida = obtenerQueryDeMenorPrioridad();
        if (!queryElegida) {
            // Carrera: no hay READY; liberar el worker y volver a esperar
            pthread_mutex_lock(&workerElegido->mutex);
            workerElegido->ocupado = false;
            pthread_mutex_unlock(&workerElegido->mutex);
            despertar_planificador();
            continue;
        }

        // Mover a EXEC y enviar al worker
        cambioEstado(&listaExecute, queryElegida);
        enviarQueryAWorker(workerElegido,queryElegida->path,queryElegida->qcb->PC,queryElegida->qcb->queryID);
    }
    return NULL;
}

// ------------------- Desalojo -------------------
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
void* evaluarDesalojo(){
    while(1){
    sem_wait(&sem_desalojo);
    query* queryReady = obtenerQueryMayorPrioridad();
    query* queryExec = obtenerQueryMenorPrioridad();

    if (queryReady && queryExec)
    {
        if (queryReady->qcb->prioridad < queryExec->qcb->prioridad)
        {
            worker* w = buscarWorkerPorQueryId(queryExec->qcb->queryID);
            enviarOpcode(DESALOJO_QUERY_PLANIFICADOR,w->socket);
            log_debug(logger,"Notificando desalojo de Query <%d>",queryExec->qcb->queryID);
        }
    }
    return NULL;
}
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
        log_debug(logger, "Se obtuvo la query ID <%d> como la de mayor prioridad.", queryMayorPrioridad->qcb->queryID);
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
// ------------------- Aging -------------------


// ------------------- Búsquedas utilitarias -------------------
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
        if (elem->workerID == idBuscado) {   // <-- antes estaba idActual
            pthread_mutex_unlock(&listaWorkers.mutex);
            return i;
        }
    }
    pthread_mutex_unlock(&listaWorkers.mutex);
    return -1;
}

void liberarWorker(worker* w){
    pthread_mutex_lock(&w->mutex);
    w->ocupado = false;
    w->idActual = -1;
    w->pathActual = NULL; 
    pthread_mutex_unlock(&w->mutex);
}



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
