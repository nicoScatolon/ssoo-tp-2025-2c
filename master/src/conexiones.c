#include "conexiones.h"
void agregarQueryControl(char* path,int socketCliente, int prioridad){

    queryControl *nuevaQueryControl = malloc(sizeof(queryControl));
    nuevaQueryControl->path = strdup(path);
    nuevaQueryControl->socket = socketCliente;
    nuevaQueryControl->prioridad = prioridad;
    nuevaQueryControl->queryControlID = generarIdQueryControl(); 
    pthread_mutex_init(&nuevaQueryControl->mutex,NULL);

    listaAdd(nuevaQueryControl,&listaQueriesControl);
    aumentarCantidadQueriesControl();

    pthread_mutex_lock(&mutex_grado);
    log_info(logger, "## Se conecta un Query Control para ejecutar la Query <%s> con prioridad <%d> - Id asignado: <%d> . Nivel multiprogramación <%d>",path,prioridad,nuevaQueryControl->queryControlID,gradoMultiprogramacion);
    pthread_mutex_unlock(&mutex_grado);
    
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
    
    aumentarGradoMultiprogramacion();
    pthread_mutex_lock(&mutex_grado);
    log_info(logger,"Conexión de Worker: ## Se conecta el Worker <%d> - Cantidad total de Workers: <%d> ",nuevoWorker->workerID,gradoMultiprogramacion);
    pthread_mutex_unlock(&mutex_grado);
       if (!esListaVacia(&listaReady)) {
        sem_post(&sem_ready);
    }
    sem_post(&sem_workers_libres);

}
void agregarQuery(char* path,int prioridad,int id){
    query *nuevaQuery = malloc(sizeof(query));
    nuevaQuery->path = strdup(path);
    qcb_t *nuevoqcb = malloc(sizeof(qcb_t));
    nuevaQuery->qcb= nuevoqcb;
    nuevaQuery->qcb->prioridad = prioridad;
    nuevaQuery->qcb->queryID = id;
    nuevaQuery->qcb->PC = 0;
    nuevaQuery->qcb->desalojoEnCurso=false;
    pthread_mutex_init(&nuevaQuery->mutex,NULL);

    listaAdd(nuevaQuery,&listaReady);
    log_debug(logger,"Se encolo query ID <%d> en READY",nuevaQuery->qcb->queryID);
    sem_post(&sem_ready);
    
    if(strcmp(configM->algoritmoPlanificacion,"PRIORIDADES") == 0 && configM->tiempoAging > 0 ){
        log_debug(logger,"Iniciando Aging para Query <%d>",id);
        pthread_t hiloAging;
        pthread_create(&hiloAging,NULL,aging,(void*)(intptr_t)id);
        pthread_detach(hiloAging);
        log_debug(logger,"Hilo  de Aging creado para Query <%d>",id);
    }
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
        printf("socketCliente desalojo %d", socketCliente);
        
        modulo moduloOrigen;
        recv(socketCliente, &moduloOrigen, sizeof(modulo), 0);
        comprobacionModulo(moduloOrigen, WORKER_DESALOJO, "WORKER_DESALOJO", operarWorkerDesalojo, socketCliente);
    }
    return NULL;
}
void* operarWorkerDesalojo(void* socketClienteVoid) {
    int socketCliente = (intptr_t)socketClienteVoid;
    
    opcode codigo = recibirOpcode(socketCliente);
    
    if(codigo < 0) {
        log_debug(logger, "Worker desconectó socket de desalojo %d", socketCliente);
        close(socketCliente);
        return NULL;
    }
    
    if (codigo != CONEXION_DESALOJO) {
        log_warning(logger, "Opcode inesperado en socket de desalojo: %d", codigo);
        close(socketCliente);
        return NULL;
    }
    
    t_paquete* paquete = recibirPaquete(socketCliente);
    if (!paquete) {
        log_error(logger, "Error recibiendo paquete en socket de desalojo %d", socketCliente);
        close(socketCliente);
        return NULL;
    }
    
    int offset = 0;
    int workerID = recibirIntDePaqueteconOffset(paquete, &offset);
    eliminarPaquete(paquete);
    
    worker* w = buscarWorkerPorId(workerID);
    if (!w) {
        log_error(logger, "No se encontró Worker con ID %d", workerID);
        close(socketCliente);
        return NULL;
    }
    
    pthread_mutex_lock(&w->mutex);
    w->socketDesalojo = socketCliente;
    pthread_mutex_unlock(&w->mutex);
    log_debug(logger, "Socket de desalojos asociado al Worker %d en socket %d", 
             workerID, socketCliente);
    
    // Ahora el hilo se queda esperando desconexión del Worker
    // while(1) {
    //     char buffer[1];
    //     int recibido = recv(socketCliente, buffer, 1, 0);
    //     if (recibido <= 0) {
    //         log_debug(logger, "Worker %d desconectó socket de desalojo", workerID);
    //         pthread_mutex_lock(&w->mutex);
    //         w->socketDesalojo = -1;
    //         pthread_mutex_unlock(&w->mutex);
    //         close(socketCliente);
    //         break;
    //     }
    // }
    
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
    int socketCliente = (intptr_t) socketClienteVoid;
    
    while(1){
        opcode codigo = recibirOpcode(socketCliente);
        
        if(codigo < 0){
            log_debug(logger,"Entro Aca - Desconexión detectada");
            queryControl * qcAEliminar = buscarQueryControlPorSocket(socketCliente);
            
            if (qcAEliminar == NULL) {
                log_error(logger,"QueryControl ya eliminado");
                exit(EXIT_FAILURE);
            }
            
            if (estaEnListaPorId(&listaReady,qcAEliminar->queryControlID)) {   
                disminuirCantidadQueriesControl();
                query* q = sacarDePorId(&listaReady,qcAEliminar->queryControlID);
                if (q == NULL)
                {
                    log_error(logger,"Error al retirar query de ready");
                    exit(EXIT_FAILURE);
                }

                // sem_wait(&sem_ready);
                eliminarQuery(q);

                pthread_mutex_lock(&mutex_grado);
                log_info(logger,"## Se desconecta un Query Control. Se finaliza la Query <%d> con prioridad <%d>. Nivel multiprocesamiento <%d>",qcAEliminar->queryControlID,qcAEliminar->prioridad,gradoMultiprogramacion);
                pthread_mutex_unlock(&mutex_grado);

                eliminarQueryControl(qcAEliminar);
                close(qcAEliminar->socket);
                goto fin_hilo;
            }
            else if(estaEnListaPorId(&listaExecute,qcAEliminar->queryControlID)){
                log_debug(logger,"Estamos dentro de execute");
                worker* workerANotificar = buscarWorkerPorQueryId(qcAEliminar->queryControlID);

                enviarOpcode(DESALOJO_QUERY_DESCONEXION,workerANotificar->socketDesalojo);
                t_paquete* paquete = crearPaquete();
                agregarIntAPaquete(paquete,qcAEliminar->queryControlID);
                enviarPaquete(paquete,workerANotificar->socketDesalojo);
                log_debug(logger,"Notificando desalojo a worker <%d> al socket <%d>",workerANotificar->workerID,workerANotificar->socketDesalojo);
                log_debug(logger,"Enviando opcode <%d>",DESALOJO_QUERY_DESCONEXION);
                eliminarPaquete(paquete);
                close(qcAEliminar->socket);

                goto fin_hilo;
            }
            else{
                log_debug(logger,"La query fue eliminada");

                disminuirCantidadQueriesControl();
                pthread_mutex_lock(&mutex_grado);
                log_info(logger,"## Se desconecta un Query Control. Se finaliza la Query <%d> con prioridad <%d>. Nivel multiprocesamiento <%d>",qcAEliminar->queryControlID,qcAEliminar->prioridad,gradoMultiprogramacion);
                pthread_mutex_unlock(&mutex_grado);
                close(qcAEliminar->socket);
                eliminarQueryControl(qcAEliminar);
                goto fin_hilo;
            }
        }
        
        switch (codigo) {
            case INICIAR_QUERY_CONTROL:{
                t_paquete* paquete = recibirPaquete(socketCliente);
                if (!paquete) {
                    log_error(logger, "Error recibiendo paquete en socket %d", socketCliente);
                    exit(EXIT_FAILURE);
                }
                int offset = 0;
                char* path = recibirStringDePaqueteConOffset(paquete, &offset);
                if (!path) {
                    log_error(logger, "Error desempaquetando path");
                    eliminarPaquete(paquete);
                    exit(EXIT_FAILURE);
                }

                int prioridad = recibirIntDePaqueteconOffset(paquete, &offset);
                agregarQueryControl(path,socketCliente,prioridad);

                free(path);
                eliminarPaquete(paquete);
                break;
            }
            default:{
                log_warning(logger, "Opcode desconocido: %d", codigo);
                goto fin_hilo;
            }
        }
    } 
    
fin_hilo:  
    return NULL;
}
void *operarWorker(void*socketClienteVoid){
    int socketCliente = (intptr_t) socketClienteVoid;
    
    while(1){
        opcode codigo = recibirOpcode(socketCliente);
        if (codigo < 0) {
            worker* w = buscarWorkerPorSocket(socketCliente);
            disminuirGradoMultiprogramacion();
            if (w == NULL) {
                log_error(logger, "Worker ya eliminado");
                exit(EXIT_FAILURE);
            }
            if (!w->ocupado) {
                pthread_mutex_lock(&mutex_grado);
                log_info(logger,"## Se desconecta el Worker <%d> - Cantidad total de Workers: <%d>",w->workerID, gradoMultiprogramacion);
                pthread_mutex_unlock(&mutex_grado);
                close(w->socketDesalojo);
                close(w->socket);

                eliminarWorker(w);
                goto fin_hilo_worker;
            }
            queryControl* qc = buscarQueryControlPorId(w->idActual);
        
            enviarOpcode(FINALIZACION_QUERY, qc->socket);
            t_paquete* paquete = crearPaquete();
            agregarStringAPaquete(paquete, "Desconexion");
            enviarPaquete(paquete, qc->socket);
            
            
            query* q = sacarDePorId(&listaExecute, qc->queryControlID);
            if (q==NULL)
            {
                log_error(logger,"Error al sacar query de Execute");
                exit(EXIT_FAILURE);
            }
            
            pthread_mutex_lock(&mutex_grado);
            log_info(logger,"## Se desconecta el Worker <%d> - Se finaliza la Query <%d> - Cantidad total de Workers: <%d>",w->workerID, q->qcb->queryID, gradoMultiprogramacion);
            pthread_mutex_unlock(&mutex_grado);
            
            log_info(logger,"## Se desaloja la Query <%d> del Worker <%d> por desconexion de worker",q->qcb->queryID, w->workerID);
            
            close(w->socketDesalojo);
            close(w->socket);

            eliminarWorker(w);
            eliminarQuery(q);
            eliminarPaquete(paquete);
            
            goto fin_hilo_worker;
        }
        switch (codigo){
            case INICIAR_WORKER:{
                t_paquete* paquete = recibirPaquete(socketCliente);
                if (!paquete) {
                    log_error(logger, "Error recibiendo paquete en socket %d", socketCliente);
                    exit(EXIT_FAILURE);
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
                    exit(EXIT_FAILURE);
                }
                int offset = 0;

                int idQuery = recibirIntDePaqueteconOffset(paqueteReceptor,&offset);
                char * file = recibirStringDePaqueteConOffset(paqueteReceptor,&offset);
                char * tag = recibirStringDePaqueteConOffset(paqueteReceptor,&offset);
                char * contenido = recibirStringDePaqueteConOffset(paqueteReceptor,&offset);
                queryControl*queryC = buscarQueryControlPorId(idQuery);
                if(queryC == NULL){
                    log_error(logger,"Hola soy el error1");
                }
                worker* workerA = buscarWorkerPorQueryId(idQuery);
                if(queryC == NULL){
                    log_error(logger,"Hola soy el error2");
                }
                enviarOpcode(LECTURA_QUERY_CONTROL,queryC->socket);
                t_paquete* paqueteEnviar = crearPaquete();
                agregarStringAPaquete(paqueteEnviar,file);
                agregarStringAPaquete(paqueteEnviar,tag);
                agregarStringAPaquete(paqueteEnviar,contenido);
                enviarPaquete(paqueteEnviar,queryC->socket);

                log_info(logger,"## Se envía un mensaje de lectura de la Query <%d> en el Worker <%d> al Query Control",idQuery, workerA->workerID);
                
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
                    exit(EXIT_FAILURE);
                }

                int offset = 0;
                int idQuery = recibirIntDePaqueteconOffset(paqueteReceptor,&offset);
                char*motivo= recibirStringDePaqueteConOffset(paqueteReceptor,&offset);
                queryControl* queryC = buscarQueryControlPorId(idQuery);
                worker* workerA = buscarWorkerPorQueryId(idQuery);
                log_debug(logger,"Motivo de finalizacion recibido: <%s>",motivo);
                
                if(motivo == NULL){
                    log_error(logger,"El motivo es de finalizacion de query es NULL");
                }

                log_info(logger, "## Se terminó la Query %d en el Worker %d", idQuery, workerA->workerID);
                liberarWorker(workerA);

                query* q = sacarDePorId(&listaExecute,idQuery);
                if (q==NULL)
                {
                    log_error(logger,"Error al sacar query de Execute");
                    exit(EXIT_FAILURE);
                }

                eliminarQuery(q);

                enviarOpcode(FINALIZACION_QUERY,queryC->socket);        
                t_paquete* paqueteEnviar = crearPaquete();
                agregarStringAPaquete(paqueteEnviar, motivo);
                enviarPaquete(paqueteEnviar, queryC->socket);
                // sem_post(&sem_ready);
                free(motivo);
                eliminarPaquete(paqueteEnviar);
                eliminarPaquete(paqueteReceptor);
                break;
            }
            
            case DESALOJO_QUERY_DESCONEXION:{
                t_paquete* paquete = recibirPaquete(socketCliente);
                int offset=0;
                int idQuery = recibirIntDePaqueteconOffset(paquete,&offset);
                int pcActualizado = recibirIntDePaqueteconOffset(paquete,&offset);

                eliminarPaquete(paquete);
                worker* workerA = buscarWorkerPorSocket(socketCliente);
                query * queryAEliminar = sacarDePorId(&listaExecute,workerA->idActual);
             
                //queryControl * queryCAEliminar = buscarQueryControlPorId(queryAEliminar->qcb->queryID);
                liberarWorker(workerA);
                log_info(logger,"## Se desaloja la Query <%d> del Worker <%d> por desconexion de queryControl",idQuery, workerA->workerID);
                //disminuirGradoMultiprogramacion();
                // pthread_mutex_lock(&mutex_grado);
                // log_info(logger,"## Se desconecta un Query Control. Se finaliza la Query <%d> con prioridad <%d>. Nivel multiprocesamiento <%d>",idQuery, queryCAEliminar->prioridad, gradoMultiprogramacion);
                // pthread_mutex_unlock(&mutex_grado);
                log_debug(logger,"query <%d> pc <%d>",idQuery,pcActualizado);
                
                // close(queryCAEliminar->socket);
                eliminarQuery(queryAEliminar);
                //eliminarQueryControl(queryCAEliminar);
                //sem_post(&sem_ready);
                break;
            }
            
            case DESALOJO_QUERY_PLANIFICADOR:{
                t_paquete* paquete = recibirPaquete(socketCliente);
                int offset = 0;
                
                int idQuery = recibirIntDePaqueteconOffset(paquete,&offset);
                log_warning(logger,"<idQuery Recibido por notificacion de desalojo> <%d>",idQuery);
                int pcActualizado = recibirIntDePaqueteconOffset(paquete,&offset);
                worker * w = buscarWorkerPorSocket(socketCliente);
                   if (w == NULL) {
        log_error(logger, "Worker no encontrado para socket %d en desalojo planificador", socketCliente);
        eliminarPaquete(paquete);
        break;
    }
                query * queryDesalojada = sacarDePorId(&listaExecute,idQuery);
                    if (queryDesalojada == NULL) {
        // La query ya fue procesada por otro hilo (finalización o desconexión)
        log_error(logger, "Query %d ya no está en EXECUTE para desalojo", idQuery);
        liberarWorker(w);
        sem_post(&sem_ready);
        eliminarPaquete(paquete);
        break;
    }
                pthread_mutex_lock(&queryDesalojada->mutex);
                queryDesalojada->qcb->PC = pcActualizado;
                queryDesalojada->qcb->desalojoEnCurso=false;
                listaAdd(queryDesalojada,&listaReady);
                pthread_mutex_unlock(&queryDesalojada->mutex);
                log_info(logger,"## Se desaloja la Query <%d> (<%d>)-del Worker <%d>Motivo:<Planificacicon>",idQuery, queryDesalojada->qcb->prioridad, w->workerID);
                liberarWorker(w);
                sem_post(&sem_ready);
                eliminarPaquete(paquete);
                break;
            }
            
            default:
                log_warning(logger, "Opcode desconocido: %d", codigo);
                goto fin_hilo_worker;
        }
    } 
    
fin_hilo_worker: 
    return NULL;
}


void aumentarCantidadQueriesControl(){
    pthread_mutex_lock(&mutex_cantidadQueriesControl);
    cantidadQueriesControl++;
    pthread_mutex_unlock(&mutex_cantidadQueriesControl);
}
void disminuirCantidadQueriesControl(){
    pthread_mutex_lock(&mutex_cantidadQueriesControl);
    cantidadQueriesControl--;
    pthread_mutex_unlock(&mutex_cantidadQueriesControl);
}
void aumentarGradoMultiprogramacion(){
     pthread_mutex_lock(&mutex_grado);
    gradoMultiprogramacion++;
    pthread_mutex_unlock(&mutex_grado);
}
void disminuirGradoMultiprogramacion(){
     pthread_mutex_lock(&mutex_grado);
    gradoMultiprogramacion--;
    pthread_mutex_unlock(&mutex_grado);
}