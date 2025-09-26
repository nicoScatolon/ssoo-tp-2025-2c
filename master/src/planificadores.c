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

static uint64_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec*1000ULL + ts.tv_nsec/1000000ULL;
}

static int prioridadActual(const query* query) {
    uint64_t tiempoEnREady = now_ms() - query->qcb->tiempoReady;
    int aging = (int)(tiempoEnREady / (uint64_t)configM->tiempoAging);
    int prioridad = query->qcb->prioridad - aging;
    return prioridad < 0 ? 0 : prioridad;
}

query* obtenerQueryDeMayorPrioridad() {
    pthread_mutex_lock(&listaReady.mutex);
    query* queryPrioritaria = list_get(listaExecute.lista, 0);
    int mayorPrioridad = queryPrioritaria->qcb->prioridad;

    for (int i = 1; i < list_size(listaReady.lista); i++) {
        query* queryComparada = list_get(listaReady.lista, i);
        int prioridadComparada = prioridadActual(queryComparada);
        if (prioridadComparada < mayorPrioridad) {
            queryPrioritaria = queryComparada;
            mayorPrioridad = prioridadComparada;
        }
    }
    pthread_mutex_unlock(&listaReady.mutex);
    return queryPrioritaria;
}

query* obtenerQueryDeMenorPrioridad() {
    pthread_mutex_lock(&listaExecute.mutex);
    query* queryElegida = list_get(listaExecute.lista, 0);
    int menorPrioridad = queryElegida->qcb->prioridad;

    for (int i = 1; i < list_size(listaExecute.lista); i++) {
        query* queryComparada = list_get(listaExecute.lista, i);
        if (queryComparada->qcb->prioridad > menorPrioridad) {
            queryElegida = queryComparada;
            menorPrioridad = queryComparada->qcb->prioridad;
        }
    }
    //list_remove_element(listaExecute.lista, queryElegida);
    pthread_mutex_unlock(&listaExecute.mutex);
    return queryElegida;
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
// Pide desalojo al worker y obtiene el PC devuelto
int solicitarDesalojoYObtenerPC(worker* w) {
    // Aviso al Worker
    enviarOpcode(DESALOJO_QUERY_PLANIFICADOR, w->socket);

    // Espero respuesta con el PC (ajustá a tu protocolo real)
    t_paquete* p = recibirPaquete(w->socket);
    if (!p) return -1;
    int offset = 0;
    int pc = recibirIntDePaqueteconOffset(p, &offset);
    eliminarPaquete(p);
    return pc;
}

// ------------------- Planificadores -------------------
void* planificadorFIFO() {

    while(1) {

        pthread_mutex_lock(&mutex_cv_planif);
        while (!hay_trabajo_para_planificar()) {
            pthread_cond_wait(&cv_planif, &mutex_cv_planif);
        }
        pthread_mutex_unlock(&mutex_cv_planif);

        worker* workerElegido = obtenerWorkerLibre();
        query* queryElegida = obtenerQuery();
        if (queryElegida == NULL) {
            log_warning(logger,"Query nula");
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
        if (!esListaVacia(&listaReady)) {
            if (hayWorkerLibre()){
                worker* workerElegido = obtenerWorkerLibre();
                query* queryElegida = obtenerQueryDeMayorPrioridad();
                list_remove_element(listaReady.lista, queryElegida);
                queryElegida->qcb->prioridad = prioridadActual(queryElegida);

                cambioEstado(&listaExecute, queryElegida);
                pthread_cond_wait(&cv_planif, &mutex_cv_planif);

                enviarQueryAWorker(workerElegido,queryElegida->path,queryElegida->qcb->PC,queryElegida->qcb->queryID);
            } else {      
                evaluarDesalojo();   
            } 

        }
        pthread_mutex_unlock(&mutex_cv_planif);

        
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

bool hay_desalojo() {
    // 1) Debe haber algo en READY
    if (esListaVacia(&listaReady)) return false;

    // 2) No debe haber workers libres
    if (hayWorkerLibre()) return false;

    if(esListaVacia(&listaExecute)) return false; //quizas no hace falta

    query* queryReady = obtenerQueryDeMayorPrioridad();
    query* queryExec = obtenerQueryDeMenorPrioridad();

    // Desalojo SI y sólo SI la READY es de mayor prioridad (número menor)
    return queryReady->qcb->prioridad < queryExec->qcb->prioridad;
}

static void aplicar_desalojo(void) {
    query* queryReady = obtenerQueryDeMayorPrioridad();
    list_remove_element(listaReady.lista, queryReady);
    queryElegida->qcb->prioridad = prioridadActual(queryReady);

    cambioEstado(&listaExecute, queryElegida);    

    query*  victimaQ = obtenerQueryDeMenorPrioridad();
    worker* victimaW = buscarWorkerPorQueryId(victimaQ->qcb->queryID);

    int pc = solicitarDesalojoYObtenerPC(victimaW);
    if (pc < 0) {
        log_warning(logger, "Fallo desalojo en Worker %d", victimaW->workerID);
        listaAdd(queryReady, &listaReady);
        nuevaQuery->qcb->tiempoReady = now_ms();
        return;
    }
    pthread_mutex_lock(&victimaQ->mutex);
    victimaQ->qcb->PC = pc;
    pthread_mutex_unlock(&victimaQ->mutex);

    query* sacada = sacarDeExecutePorId(victimaQ->qcb->queryID);
    if (sacada) listaAdd(sacada, &listaReady);

    log_info(logger, "## Se desaloja la Query %d (%d) y comienza a ejecutar la Query %d (%d) en el Worker %d", victimaQ->qcb->queryID, victimaQ->qcb->prioridad, queryReady->qcb->queryID, queryReady->qcb->prioridad, victimaW->workerID);

    cambioEstado(&listaExecute, queryReady);
    enviarQueryAWorker(victimaW, queryReady->path, queryReady->qcb->PC, queryReady->qcb->queryID);

    //Por si quedaron más READY con chance de preempción
    despertar_planificador();
}

void* evaluarDesalojo() {
    while(1) {
        pthread_mutex_lock(&mutex_cv_planif);
        while (!hay_desalojo()) {
            pthread_cond_wait(&cv_planif, &mutex_cv_planif);
        }
        pthread_mutex_unlock(&mutex_cv_planif);

        aplicar_desalojo();
    }
    return NULL;
}

// ------------------- Aging -------------------
void* aging() {
    while (1) {
        usleep(configM->tiempoAging * 1000); 
        pthread_mutex_lock(&listaReady.mutex);
        for (int i = 0; i < list_size(listaReady.lista); i++) {
            query* q = list_get(listaReady.lista, i);
            int old = q->qcb->prioridad;
            if (old > 0) {
                q->qcb->prioridad = old - 1;
                log_info(logger, "## <%d> Cambio de prioridad: <%d> - <%d>", q->qcb->queryID, old, q->qcb->prioridad); 
            }
        }
        pthread_mutex_unlock(&listaReady.mutex);
        despertar_planificador();
    }
    return NULL;
}

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