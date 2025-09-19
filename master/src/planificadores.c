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

bool hay_desalojo_necesario() {
    // 1) Debe haber algo en READY
    if (esListaVacia(&listaReady)) return false;

    // 2) No debe haber workers libres
    if (hayWorkerLibre()) return false;

    // 3) Mejor READY (número de prioridad más chico)
    int pr_ready = 0;
    pthread_mutex_lock(&listaReady.mutex);
    query* best = NULL;
    for (int i = 0; i < list_size(listaReady.lista); i++) {
        query* q = list_get(listaReady.lista, i);
        if (!best || q->qcb->prioridad < best->qcb->prioridad) best = q;
    }
    if (!best) { pthread_mutex_unlock(&listaReady.mutex); return false; }
    pr_ready = best->qcb->prioridad;
    pthread_mutex_unlock(&listaReady.mutex);

    // 4) Peor en EXEC (número de prioridad más grande)
    int pr_worst_exec = -1;
    bool hay_exec = false;

    pthread_mutex_lock(&listaWorkers.mutex);
    pthread_mutex_lock(&listaExecute.mutex);
    for (int i = 0; i < list_size(listaWorkers.lista); i++) {
        worker* w = list_get(listaWorkers.lista, i);

        pthread_mutex_lock(&w->mutex);
        bool ocupado = w->ocupado;
        int idActual = w->idActual;
        pthread_mutex_unlock(&w->mutex);

        if (!ocupado) continue;
        // buscar su query actual en EXEC
        for (int j = 0; j < list_size(listaExecute.lista); j++) {
            query* q = list_get(listaExecute.lista, j);
            if (q->qcb->queryID == w->idActual) {
                hay_exec = true;
                if (q->qcb->prioridad > pr_worst_exec) pr_worst_exec = q->qcb->prioridad;
                break;
            }
        }
    }
    pthread_mutex_unlock(&listaExecute.mutex);
    pthread_mutex_unlock(&listaWorkers.mutex);

    if (!hay_exec) return false;

    // Desalojo SI y sólo SI la READY es de mayor prioridad (número menor)
    return pr_ready < pr_worst_exec;
}

static void aplicar_desalojo(void) {
    // 1) Elegir y RETIRAR la mejor READY (menor número = mayor prioridad)
    int   pr_best;
    query* best;

    pthread_mutex_lock(&listaReady.mutex);
    if (list_is_empty(listaReady.lista)) { pthread_mutex_unlock(&listaReady.mutex); return; }

    int idx_best = 0;
    best = list_get(listaReady.lista, 0);
    for (int i = 1; i < list_size(listaReady.lista); i++) {
        query* q = list_get(listaReady.lista, i);
        if (q->qcb->prioridad < best->qcb->prioridad) { best = q; idx_best = i; }
    }
    best = list_remove(listaReady.lista, idx_best);   // ← ya la quitamos de READY bajo el mismo lock
    pthread_mutex_unlock(&listaReady.mutex);

    if (!best) return;           // defensa por carrera
    pr_best = best->qcb->prioridad;

    // 2) Encontrar la PEOR en EXEC (mayor número) y su worker
    worker* victimaW = NULL;
    query*  victimaQ = NULL;
    int     pr_victima = -1;

    pthread_mutex_lock(&listaWorkers.mutex);
    pthread_mutex_lock(&listaExecute.mutex);
    for (int i = 0; i < list_size(listaWorkers.lista); i++) {
        worker* w = list_get(listaWorkers.lista, i);

        pthread_mutex_lock(&w->mutex);
        bool ocupado = w->ocupado;
        int idActual = w->idActual;
        pthread_mutex_unlock(&w->mutex);

        if (!ocupado) continue;
        for (int j = 0; j < list_size(listaExecute.lista); j++) {
            query* q = list_get(listaExecute.lista, j);
            if (q->qcb->queryID == w->idActual) {
                if (q->qcb->prioridad > pr_victima) { 
                    pr_victima = q->qcb->prioridad; 
                    victimaW = w; 
                    victimaQ = q; 
                }
                break;
            }
        }
    }
    pthread_mutex_unlock(&listaExecute.mutex);
    pthread_mutex_unlock(&listaWorkers.mutex);

    if (!victimaW || !victimaQ) {
        // No hay ejecutando: devolvemos 'best' a READY
        listaAdd(best, &listaReady);
        return;
    }

    // 3) Verificar condición de desalojo (estricta: menor número = mayor prioridad)
    if (!(pr_best < victimaQ->qcb->prioridad)) {
        // No corresponde: devolvemos 'best' a READY (al final)
        listaAdd(best, &listaReady);
        return;
    }

    // 4) Pedir desalojo y actualizar PC de la víctima
    int pc = solicitarDesalojoYObtenerPC(victimaW);
    if (pc < 0) {
        log_warning(logger, "Fallo desalojo en Worker %d", victimaW->workerID);
        listaAdd(best, &listaReady);
        return;
    }
    pthread_mutex_lock(&victimaQ->mutex);
    victimaQ->qcb->PC = pc;
    pthread_mutex_unlock(&victimaQ->mutex);

    // 5) Mover víctima EXEC -> READY
    query* sacada = sacarDeExecutePorId(victimaQ->qcb->queryID);
    if (sacada) listaAdd(sacada, &listaReady);

    // 6) Enviar 'best' a EXEC en el mismo worker
    log_info(logger,
        "## Se desaloja la Query %d (%d) y comienza a ejecutar la Query %d (%d) en el Worker %d",
        victimaQ->qcb->queryID, pr_victima, best->qcb->queryID, pr_best, victimaW->workerID);

    cambioEstado(&listaExecute, best);
    enviarQueryAWorker(victimaW, best->path, best->qcb->PC, best->qcb->queryID);

    // 7) Por si quedaron más READY con chance de preempción
    despertar_planificador();
}

void* evaluarDesalojo() {
    while(1) {
        pthread_mutex_lock(&mutex_cv_planif);
        while (!hay_desalojo_necesario()) {
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

/*void* planificadorPrioridad() {

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
}*/


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
