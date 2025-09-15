#include "utils/config.h"
#include "utils/logs.h"
#include "utils/paquete.h"
#include "globals.h"
#include "conexiones.h"

// Buscar QC por socket
static queryControl* buscarQueryControlPorSocket(int sock){
    pthread_mutex_lock(&listaQueriesControl.mutex);
    for (int i = 0; i < list_size(listaQueriesControl.lista); i++) {
        queryControl* qc = list_get(listaQueriesControl.lista, i);
        if (qc->socket == sock) {
            pthread_mutex_unlock(&listaQueriesControl.mutex);
            return qc;
        }
    }
    pthread_mutex_unlock(&listaQueriesControl.mutex);
    return NULL;
}

// Buscar Worker por socket
static worker* buscarWorkerPorSocket(int sock){
    pthread_mutex_lock(&listaWorkers.mutex);
    for (int i = 0; i < list_size(listaWorkers.lista); i++) {
        worker* w = list_get(listaWorkers.lista, i);
        if (w->socket == sock) {
            pthread_mutex_unlock(&listaWorkers.mutex);
            return w;
        }
    }
    pthread_mutex_unlock(&listaWorkers.mutex);
    return NULL;
}

// Sacar de EXEC por queryID (igual a la estática que tenés en planificadores.c)
static query* sacarDeExecutePorId_conex(int queryID) {
    query* q = NULL;
    pthread_mutex_lock(&listaExecute.mutex);
    for (int i = 0; i < list_size(listaExecute.lista); i++) {
        query* it = list_get(listaExecute.lista, i);
        if (it->qcb->queryID == queryID) {
            q = list_remove(listaExecute.lista, i);
            break;
        }
    }
    pthread_mutex_unlock(&listaExecute.mutex);
    return q;
}


void agregarQueryControl(char* path,int socketCliente, int prioridad){

    queryControl *nuevaQueryControl = malloc(sizeof(queryControl));
    nuevaQueryControl->path = strdup(path);
    nuevaQueryControl->socket = socketCliente;
    nuevaQueryControl->prioridad = prioridad;
    nuevaQueryControl->queryControlID = generarIdQueryControl(); 
    pthread_mutex_init(&nuevaQueryControl->mutex,NULL);

    listaAdd(nuevaQueryControl,&listaQueriesControl);

    pthread_mutex_lock(&mutex_cantidadQueriesControl);
    cantidadQueriesControl++;
    pthread_mutex_unlock(&mutex_cantidadQueriesControl);

    log_info(logger, "## Se conecta un Query Control para ejecutar la Query <PATH_QUERY> <%s> con prioridad <PRIORIDAD> <%d> - Id asignado: <QUERY_ID> <%d> . Nivel multiprogramación <CANTIDAD> <%d>",path,prioridad,nuevaQueryControl->queryControlID,cantidadQueriesControl);
    agregarQuery(path,prioridad,nuevaQueryControl->queryControlID);
}
void agregarWorker(int socketCliente,int idWorker){

    worker *nuevoWorker = malloc(sizeof(worker));
    nuevoWorker->pathActual = NULL;
    nuevoWorker->socket = socketCliente;
    nuevoWorker->ocupado = false;
    nuevoWorker->workerID = idWorker;
    nuevoWorker->idActual = -1;
    pthread_mutex_init(&nuevoWorker->mutex,NULL);

    listaAdd(nuevoWorker,&listaWorkers);
    despertar_planificador();

    pthread_mutex_lock(&mutex_grado);
    gradoMultiprogramacion++;
    pthread_mutex_unlock(&mutex_grado);

    log_info(logger,"Conexión de Worker: ## Se conecta el Worker <WORKER_ID> <%d> - Cantidad total de Workers: <CANTIDAD> <%d> ",nuevoWorker->workerID,gradoMultiprogramacion);

}
void agregarQuery(char* path,int prioridad,int id){
    query *nuevaQuery = malloc(sizeof(query));
    nuevaQuery->path = strdup(path);
    qcb_t *nuevoqcb = malloc(sizeof(qcb_t));
    nuevaQuery->qcb= nuevoqcb;
    nuevaQuery->qcb->prioridad = prioridad;
    nuevaQuery->qcb->queryID = id;
    nuevaQuery->qcb->PC = 0;
    pthread_mutex_init(&nuevaQuery->mutex,NULL);

    listaAdd(nuevaQuery,&listaReady);
    despertar_planificador();
    log_debug(logger,"Se encolo query ID <%d> en READY",nuevaQuery->qcb->queryID);
}

void eliminarQueryControl(queryControl* queryC){

    int posicion = obtenerPosicionQCPorId(queryC->queryControlID);
    pthread_mutex_lock(&listaQueriesControl.mutex);
    list_remove(listaQueriesControl.lista,posicion);
    pthread_mutex_unlock(&listaQueriesControl.mutex);

    pthread_mutex_destroy(&queryC->mutex);
    free(queryC->path);
    free(queryC);

    pthread_mutex_lock(&mutex_cantidadQueriesControl);
    cantidadQueriesControl--;
    pthread_mutex_unlock(&mutex_cantidadQueriesControl);
}
void eliminarWorker(worker* workerEliminar){
    int posicion = obtenerPosicionWPorId(workerEliminar->workerID);

    pthread_mutex_lock(&listaWorkers.mutex);
    if (posicion >= 0) list_remove(listaWorkers.lista,posicion);
    pthread_mutex_unlock(&listaWorkers.mutex);

    pthread_mutex_destroy(&workerEliminar->mutex);
    // No free(workerEliminar->pathActual);  // ← no hacemos strdup al asignar
    workerEliminar->pathActual = NULL;
    free(workerEliminar);

    pthread_mutex_lock(&mutex_grado);
    gradoMultiprogramacion--;
    pthread_mutex_unlock(&mutex_grado);
}
void eliminarQuery(query * queryEliminar){
    pthread_mutex_destroy(&queryEliminar->mutex);
    free(queryEliminar->path);
    free(queryEliminar->qcb);
    free(queryEliminar);
}
uint32_t generarIdQueryControl() {
    pthread_mutex_lock(&mutex_id_queryControl);
    uint32_t id = siguienteIdQueryControl++;
    pthread_mutex_unlock(&mutex_id_queryControl);
    return id;
}

void establecerConexiones(){
    char* puertoQueryControl = string_itoa(configM->puertoEscuchaQueryControl);
    char* puertoWorker = string_itoa(configM->puertoEscuchaWorker);
    
    int socketQueryControl = iniciarServidor(puertoQueryControl,logger,"MASTER_QUERYCONTROL");
    int socketWorker = iniciarServidor(puertoWorker,logger,"MASTER_WORKER");
    //int socketWorker= iniciarServidor(puertoWorker,logger,"MASTER");
    free(puertoWorker);
    free(puertoQueryControl);

    
    pthread_t hiloQueryControl;
    pthread_t hiloWorker;
    pthread_create(&hiloQueryControl,NULL,escucharQueryControl,(void*)(intptr_t)socketQueryControl);
    pthread_create(&hiloWorker,NULL,escucharWorker,(void*)(intptr_t)socketWorker);

    pthread_detach(hiloQueryControl);
    pthread_detach(hiloWorker);
}

void *escucharQueryControl(void* socketServidorVoid){
    int socketServidor = (intptr_t) socketServidorVoid;
    log_debug(logger,"Servidor MASTER_QUERY_CONTROL escuchando conexiones");
    while (1)
    {
        int socketCliente = esperarCliente(socketServidor,logger);
        printf("socketCliente %d",socketCliente);
        modulo moduloOrigen;
        recv(socketCliente,&moduloOrigen,sizeof(modulo),0);
        comprobacionModulo(moduloOrigen,QUERY_CONTROL,"QUERYCONTROL",operarQueryControl,socketCliente);
    }
    return NULL;
}
void *escucharWorker(void* socketServidorVoid){
    int socketServidor = (intptr_t) socketServidorVoid;
    log_debug(logger,"Servidor MASTER_WORKER escuchando conexiones");
    while (1)
    {
        int socketCliente = esperarCliente(socketServidor,logger);
        printf("socketCliente %d",socketCliente);
        modulo moduloOrigen;
        recv(socketCliente,&moduloOrigen,sizeof(modulo),0);
        comprobacionModulo(moduloOrigen,WORKER,"WORKER",operarWorker,socketCliente);
    }
    return NULL;
}
void comprobacionModulo(modulo modulo_origen, modulo esperado, char *modulo, void*(*operacion)(void*), int socket_cliente)
{
    if (modulo_origen == esperado)
    {
        log_debug(logger, "Se conecto %s", modulo);
        pthread_t hilo_operacion;
        //log_debug(logger,"Aca no se rompio");
        if (pthread_create(&hilo_operacion, NULL, operacion, (void*)(intptr_t)socket_cliente) != 0) {
            log_error(logger, "No se pudo crear el hilo para %d", esperado);
            close(socket_cliente);
        }
        //log_debug(logger,"Se rompio antes del detach");
        pthread_detach(hilo_operacion); // Operaciones de modulos
    }
    else
    {
        log_warning(logger, "No es %s", modulo);
        close(socket_cliente);
    }
}
void * operarQueryControl(void* socketClienteVoid){
    int socketCliente =(intptr_t) socketClienteVoid;
    //log_debug(logger,"Llego a operarQueryControl");
    while(1){
        opcode codigo = recibirOpcode(socketCliente);
        if (codigo < 0) {
            queryControl* qc = buscarQueryControlPorSocket(socketCliente);
            if (qc) {
                log_info(logger,
                    "## Se desconecta un Query Control para ejecutar la Query <PATH_QUERY> <%s> con prioridad <PRIORIDAD> <%d>. Nivel multiprogramación <CANTIDAD> <%d>",
                    qc->path, qc->prioridad, cantidadQueriesControl - 1);

                eliminarQueryControl(qc);  // ya decrementa cantidadQueriesControl y lo saca de la lista
            }

            log_warning(logger, "Cliente desconectado en socket %d", socketCliente);
            close(socketCliente);
            return NULL;
        }

        switch (codigo)
        {
        case INICIAR_QUERY_CONTROL:{
            t_paquete* paquete = recibirPaquete(socketCliente);
            if (!paquete) {
                log_error(logger, "Error recibiendo paquete en socket %d", socketCliente);
                break;
            }

            int offset = 0;
            char* path = recibirStringDePaqueteConOffset(paquete, &offset);
            if (!path) {
                log_error(logger, "Error desempaquetando path");
                eliminarPaquete(paquete);
                break;
            }
            int prioridad = recibirIntDePaqueteconOffset(paquete, &offset);

            agregarQueryControl(path,socketCliente,prioridad);
            free(path);
            eliminarPaquete(paquete);

            break;
        }
        case DESCONEXION_QUERY_CONTROL:{
            
        }
        default:
            log_warning(logger, "Opcode desconocido: %d", codigo);
            return NULL;
            break;
        }
    }
    close(socketCliente);
    return NULL;
}
void *operarWorker(void*socketClienteVoid){
    int socketCliente = (intptr_t) socketClienteVoid;
    while(1){
        opcode codigo = recibirOpcode(socketCliente);
            if (codigo < 0) {
                worker* w = buscarWorkerPorSocket(socketCliente);
                if (w) {
                    // tomar estado actual con su mutex
                    pthread_mutex_lock(&w->mutex);
                    bool estabaOcupado = w->ocupado;
                    int qid = w->idActual;
                    int wid = w->workerID;
                    pthread_mutex_unlock(&w->mutex);

                    // log desconexión
                    log_info(logger, "## Se desconecta el Worker %d - Cantidad total de Workers: <CANTIDAD> <%d>",
                            wid, gradoMultiprogramacion - 1);

                    // si estaba ejecutando, log y reencolar la query (o EXIT si así lo definiste)
                    if (estabaOcupado) {
                        log_info(logger, "## Se desaloja la Query %d del Worker %d", qid, wid);
                        query* q = sacarDeExecutePorId_conex(qid);
                        if (q) listaAdd(q, &listaReady);
                        despertar_planificador();
                    }

                    eliminarWorker(w);  // saca de lista y decrementa gradoMultiprogramacion
                }

                log_warning(logger, "Cliente desconectado en socket %d", socketCliente);
                close(socketCliente);
                return NULL;
            }

        }
    switch (codigo)
    {
    case INICIAR_WORKER:{
        t_paquete* paquete = recibirPaquete(socketCliente);
        if (!paquete) {
                log_error(logger, "Error recibiendo paquete en socket %d", socketCliente);
                break;
            }
            int offset = 0;
            int idWorker = recibirIntDePaqueteconOffset(paquete,&offset);
            agregarWorker(socketCliente,idWorker);
            eliminarPaquete(paquete);
            break;
    }
    case LECTURA_QUERY_CONTROL:{
        t_paquete* paqueteReceptor = recibirPaquete(socketCliente);
        if (!paqueteReceptor){
                log_error(logger, "Error recibiendo paquete en socket %d", socketCliente);
                break;
        }
            int offset = 0;
            int idQuery = recibirIntDePaqueteconOffset(paqueteReceptor,&offset);
            char * file = recibirStringDePaqueteConOffset(paqueteReceptor,&offset);
            char * tag = recibirStringDePaqueteConOffset(paqueteReceptor,&offset);
            char * contenido = recibirStringDePaqueteConOffset(paqueteReceptor,&offset);
            queryControl*queryC = buscarQueryControlPorId(idQuery);
            worker* workerA = buscarWorkerPorId(idQuery);
            t_paquete* paqueteEnviar = crearPaquete();

            agregarStringAPaquete(paqueteEnviar,file);
            agregarStringAPaquete(paqueteEnviar,tag);
            agregarStringAPaquete(paqueteEnviar,contenido);
            enviarPaquete(paqueteEnviar,queryC->socket);
            log_info(logger,"## Se envía un mensaje de lectura de la Query <%d> en el Worker <%d> al Query Control",idQuery,workerA->workerID);
            

            free(file);
            free(tag);
            free(contenido);
            eliminarPaquete(paqueteReceptor);
            eliminarPaquete(paqueteEnviar);

            break;
    }
    case FINALIZACION_QUERY: {
        t_paquete * paqueteReceptor = recibirPaquete(socketCliente);
        if (!paqueteReceptor){
            log_error(logger, "Error recibiendo paquete en socket %d", socketCliente);
            break;
        }
        int offset = 0;
        int idQuery = recibirIntDePaqueteconOffset(paqueteReceptor,&offset);
        char * motivo = recibirStringDePaqueteConOffset(paqueteReceptor,&offset);

        queryControl* queryC = buscarQueryControlPorId(idQuery);
        worker* workerA = buscarWorkerPorId(idQuery);

        if (workerA) {
            log_info(logger, "## Se terminó la Query %d en el Worker %d", idQuery, workerA->workerID);
        }

        // mover EXEC -> EXIT
        query* q = sacarDeExecutePorId_conex(idQuery);
        if (q) listaAdd(q, &listaExit);

        // liberar worker y despertar planificador
        if (workerA) liberarWorker(workerA);
        despertar_planificador();

        // notificar al QC
        if (queryC) {
            t_paquete* paqueteEnviar = crearPaquete();
            agregarStringAPaquete(paqueteEnviar, motivo);
            enviarPaquete(paqueteEnviar, queryC->socket);
            eliminarPaquete(paqueteEnviar);

            eliminarQueryControl(queryC);
        }

        free(motivo);
        eliminarPaquete(paqueteReceptor);
        break;
    }    
    default:
        break;
    }
}

