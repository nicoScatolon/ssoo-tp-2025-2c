#include "conexiones.h"


queryControl* buscarQueryControlPorSocket(int socket){
    pthread_mutex_lock(&listaQueriesControl.mutex);
    for (int i = 0; i < list_size(listaQueriesControl.lista); i++) {
        queryControl* qc = list_get(listaQueriesControl.lista, i);
        if (qc->socket == socket) {
            pthread_mutex_unlock(&listaQueriesControl.mutex);
            return qc;
        }
    }
    pthread_mutex_unlock(&listaQueriesControl.mutex);
    return NULL;
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
    nuevoWorker->socketDesalojo = -1;
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

    //log_info(logger,"## Se desconecta un Query Control. Se finaliza la Query <%d> con prioridad <%d>. Cantidad de QueriesControl <%d>",queryC->queryControlID,queryC->prioridad,cantidadQueriesControl);

    pthread_mutex_destroy(&queryC->mutex);
    free(queryC->path);
    free(queryC);

}
void reducirCantidadQueryControl(){
    pthread_mutex_lock(&mutex_cantidadQueriesControl);
    cantidadQueriesControl--;
    pthread_mutex_unlock(&mutex_cantidadQueriesControl);
}
void reducirGradoMultiprogramacion(){
    pthread_mutex_lock(&mutex_grado);
    gradoMultiprogramacion--;
    pthread_mutex_unlock(&mutex_grado);
}
void eliminarWorker(worker* workerEliminar){
    int posicion = obtenerPosicionWPorId(workerEliminar->workerID);

    pthread_mutex_lock(&listaWorkers.mutex);
    list_remove(listaWorkers.lista,posicion);
    pthread_mutex_unlock(&listaWorkers.mutex);


    //log_info(logger,"## Se desconecta el Worker <%d> - Se finaliza la Query <%d> - Cantidad total de Workers: <%d>",workerEliminar->workerID,workerEliminar->idActual,gradoMultiprogramacion);

    pthread_mutex_destroy(&workerEliminar->mutex);
    free(workerEliminar->pathActual);
    free(workerEliminar);
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
    char* puertoDesalojo = string_itoa(configM->puertoEscuchaDesalojo);

    int socketQueryControl = iniciarServidor(puertoQueryControl,logger,"MASTER_QUERYCONTROL");
    int socketWorker = iniciarServidor(puertoWorker,logger,"MASTER_WORKER");
    int socketDesalojo = iniciarServidor(puertoDesalojo,logger,"MASTER_WORKER_DESALOJO");

    free(puertoWorker);
    free(puertoQueryControl);
    free(puertoDesalojo);

    pthread_t hiloQueryControl,hiloWorker,hiloDesalojo;

    pthread_create(&hiloQueryControl,NULL,escucharQueryControl,(void*)(intptr_t)socketQueryControl);
    pthread_create(&hiloWorker,NULL,escucharWorker,(void*)(intptr_t)socketWorker);
    pthread_create(&hiloDesalojo,NULL,escucharWorkerDesalojo,(void*)(intptr_t)socketDesalojo);

    pthread_detach(hiloQueryControl);
    pthread_detach(hiloWorker);
    pthread_detach(hiloDesalojo);
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
void* escucharWorkerDesalojo(void* socketServidorVoid) {
    int socketServidor = (intptr_t)socketServidorVoid;
    log_debug(logger, "Servidor MASTER_DESALOJO escuchando conexiones");
    
    while (1) {
        int socketCliente = esperarCliente(socketServidor, logger);
        log_debug(logger, "Worker conectado al servidor de desalojos en socket %d", socketCliente);
        
        modulo moduloOrigen;
        recv(socketCliente, &moduloOrigen, sizeof(modulo), 0);
        
        if (moduloOrigen == WORKER) {
            t_paquete* paquete = recibirPaquete(socketCliente);
            int offset = 0;
            int workerID = recibirIntDePaqueteconOffset(paquete, &offset);
            eliminarPaquete(paquete);
            
            // Buscar el worker y asignarle este socket de desalojos
            worker* w = buscarWorkerPorId(workerID);
            if (w) {
                pthread_mutex_lock(&w->mutex);
                w->socketDesalojo = socketCliente; 
                pthread_mutex_unlock(&w->mutex);
                log_debug(logger, "Socket de desalojos asociado al Worker %d", workerID);
            } else {
                log_error(logger, "No se encontró Worker con ID %d", workerID);
                close(socketCliente);
            }
        } else {
            log_warning(logger, "Módulo incorrecto conectado al servidor de desalojos");
            close(socketCliente);
        }
    }
    return NULL;
}
void comprobacionModulo(modulo modulo_origen, modulo esperado, char *modulo, void*(*operacion)(void*), int socket_cliente)
{
    if (modulo_origen == esperado)
    {
        log_debug(logger, "Se conecto %s", modulo);
        pthread_t hilo_operacion;
        if (pthread_create(&hilo_operacion, NULL, operacion, (void*)(intptr_t)socket_cliente) != 0) {
            log_error(logger, "No se pudo crear el hilo para %d", esperado);
            close(socket_cliente);
        }
        pthread_detach(hilo_operacion); 
    }
    else
    {
        log_warning(logger, "No es %s", modulo);
        close(socket_cliente);
    }
}
void * operarQueryControl(void* socketClienteVoid){
    int socketCliente =(intptr_t) socketClienteVoid;
    while(1){
        opcode codigo = recibirOpcode(socketCliente);
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
            queryControl * qcAEliminar = buscarQueryControlPorSocket(socketCliente);
            if (estaEnListaPorId(&listaReady,qcAEliminar->queryControlID))
            {   
                reducirCantidadQueryControl();
                query* q = sacarDePorId(&listaReady,qcAEliminar->queryControlID);
                log_info(logger,"## Se desconecta un Query Control. Se finaliza la Query <%d> con prioridad <%d>. Nivel multiprocesamiento <%d>",qcAEliminar->queryControlID,qcAEliminar->prioridad,cantidadQueriesControl);
                eliminarQuery(q);
                eliminarQueryControl(qcAEliminar);
                close(socketCliente);

            }
            else if(estaEnListaPorId(&listaExecute,qcAEliminar->queryControlID)){
                worker* workerANotificar = buscarWorkerPorQueryId(qcAEliminar->queryControlID);
                enviarOpcode(DESALOJO_QUERY_DESCONEXION,workerANotificar->socket);
            }
             break;
        }
        default:
            log_warning(logger, "Opcode desconocido: %d", codigo);
            return NULL;
            break;
        }
    }
    return NULL;
}
void *operarWorker(void*socketClienteVoid){
    int socketCliente = (intptr_t) socketClienteVoid;
    while(1){
        opcode codigo = recibirOpcode(socketCliente);
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
            worker* workerA = buscarWorkerPorQueryId(idQuery);

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


        queryControl* queryC = buscarQueryControlPorId(idQuery);
        worker* workerA = buscarWorkerPorQueryId(idQuery);

        log_info(logger, "## Se terminó la Query %d en el Worker %d", idQuery, workerA->workerID);

        query* q = sacarDePorId(&listaExecute,idQuery);

        liberarWorker(workerA);
        despertar_planificador();
        reducirCantidadQueryControl();

        enviarOpcode(FINALIZACION_QUERY,queryC->socket);
        t_paquete* paqueteEnviar = crearPaquete();
        agregarStringAPaquete(paqueteEnviar, "Finalizacion");
        enviarPaquete(paqueteEnviar, queryC->socket);

        log_info(logger,"## Se desconecta un Query Control. Se finaliza la Query <%d> con prioridad <%d>. Nivel multiprocesamiento <%d>",q->qcb->queryID,q->qcb->prioridad,cantidadQueriesControl);
        close(queryC->socket);
        eliminarQueryControl(queryC);
        eliminarQuery(q);
        eliminarPaquete(paqueteEnviar);
        eliminarPaquete(paqueteReceptor);
        break;
    }    
       case DESCONEXION_WORKER:{
            worker* w = buscarWorkerPorSocket(socketCliente);
            reducirGradoMultiprogramacion();
            if (!w->ocupado)
            {
                
                log_info(logger,"## Se desconecta el Worker <%d> - Cantidad total de Workers: <%d>",w->workerID,gradoMultiprogramacion);
                eliminarWorker(w);
                close(socketCliente);
                break;
            }

            queryControl* qc = buscarQueryControlPorId(w->idActual);
            reducirCantidadQueryControl();
            
            enviarOpcode(FINALIZACION_QUERY,qc->socket);
            t_paquete* paquete= crearPaquete();
            agregarStringAPaquete(paquete,"Desconexion");
            enviarPaquete(paquete,qc->socket);

            query* q = sacarDePorId(&listaExecute,qc->queryControlID);
            log_info(logger,"## Se desconecta un Query Control. Se finaliza la Query <%d> con prioridad <%d>. Nivel multiprocesamiento <%d>",q->qcb->queryID,q->qcb->prioridad,cantidadQueriesControl);
            log_info(logger,"## Se desaloja la Query <%d> del Worker <%d> por desconexion de worker",q->qcb->queryID,w->workerID);
            log_info(logger,"## Se desconecta el Worker <%d> - Se finaliza la Query <%d> - Cantidad total de Workers: <%d>",w->workerID,q->qcb->queryID,gradoMultiprogramacion);

            close(qc->socket);
            close(socketCliente);
            
            eliminarWorker(w);
            eliminarQuery(q);
            eliminarQueryControl(qc);
            eliminarPaquete(paquete);

            break;
        }
        case DESALOJO_QUERY_DESCONEXION:{
            worker* workerA = buscarWorkerPorSocket(socketCliente);
            query * queryAEliminar = sacarDePorId(&listaExecute,workerA->idActual);
            queryControl * queryCAEliminar = buscarQueryControlPorId(queryAEliminar->qcb->queryID);
            liberarWorker(workerA);
            log_info(logger,"## Se desaloja la Query <%d> del Worker <%d> por desconexion de queryControl",queryAEliminar->qcb->queryID,workerA->workerID);
            reducirGradoMultiprogramacion();
            log_info(logger,"## Se desconecta un Query Control. Se finaliza la Query <%d> con prioridad <%d>. Nivel multiprocesamiento <%d>",queryCAEliminar->queryControlID,queryCAEliminar->prioridad,cantidadQueriesControl);

            close(queryCAEliminar->socket);
            eliminarQuery(queryAEliminar);
            eliminarQueryControl(queryCAEliminar);
            break;
        }
        case DESALOJO_QUERY_PLANIFICADOR:{
            t_paquete* paquete =recibirPaquete(socketCliente);
            int offset = 0;
            
            int idQuery = recibirIntDePaqueteconOffset(paquete,&offset);
            int pcActualizado = recibirIntDePaqueteconOffset(paquete,&offset);
            worker * w = buscarWorkerPorSocket(socketCliente);
            query * queryDesalojada = sacarDePorId(&listaExecute,idQuery);
            queryDesalojada->qcb->PC = pcActualizado;

            listaAdd(queryDesalojada,&listaReady);
            log_info(logger,"## Se desaloja la Query <%d> (<%d>)-del Worker <%d>Motivo:<Planificacicon>",queryDesalojada->qcb->queryID,queryDesalojada->qcb->prioridad,w->workerID);
            liberarWorker(w);
            despertar_planificador();
            eliminarPaquete(paquete);
            break;
        }
    default:
        break;
    }
    }
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