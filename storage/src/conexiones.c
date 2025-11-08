#include "conexiones.h"

void establecerConexionesStorage(void){
    char* puertoEscucha = string_itoa(configS->puertoEscucha);
    int socketEscucha = iniciarServidor(puertoEscucha,logger,"STORAGE");
    free(puertoEscucha);

    pthread_t hiloStorage;
    pthread_create(&hiloStorage,NULL,escucharWorkers,(void*)(intptr_t)socketEscucha);
    pthread_detach(hiloStorage);
}


void *escucharWorkers(void* socketServidorVoid){
    int socketServidor = (intptr_t) socketServidorVoid;
    log_debug(logger,"Servidor STORAGE escuchando conexiones");
    while (1)
    {
        int socketCliente = esperarCliente(socketServidor,logger);
        printf("socketCliente %d",socketCliente);
        modulo moduloOrigen;
        recv(socketCliente,&moduloOrigen,sizeof(modulo),0);
        comprobacionModulo(moduloOrigen,WORKER,"WORKER",operarWorkers,socketCliente);
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

void *operarWorkers(void*socketClienteVoid){
    int socketCliente = (intptr_t) socketClienteVoid;
    while(1){
        opcode codigo = recibirOpcode(socketCliente);
        if (codigo < 0) {
            int workerId = obtenerYRemoverWorker(socketCliente);
            decrementarWorkers(workerId);
            close(socketCliente);
            break;
        }
        switch (codigo)
        {
        case HANDSHAKE_STORAGE_WORKER:{
            log_debug(logger,"Se recibio OPCODE");
            t_paquete*paqueteRecibir = recibirPaquete(socketCliente);
            int offset = 0;
            int workerId = recibirIntDePaqueteconOffset(paqueteRecibir,&offset);

            incrementarWorkers(workerId);
            registrarWorker(socketCliente,workerId);
            
            t_paquete* paqueteEnviar = crearPaquete();
            enviarOpcode(HANDSHAKE_STORAGE_WORKER,socketCliente);
            agregarIntAPaquete(paqueteEnviar,configSB->FS_SIZE);
            agregarIntAPaquete(paqueteEnviar,configSB->BLOCK_SIZE);
            enviarPaquete(paqueteEnviar,socketCliente);

            eliminarPaquete(paqueteEnviar);
            eliminarPaquete(paqueteRecibir);
            log_debug(logger,"Enviando datos FS_SIZE <%d> BLOCK_SIZE <%d>",configSB->FS_SIZE,configSB->BLOCK_SIZE);
            break;
        }
        case CREATE_FILE:{
            log_debug(logger,"Se recibio OPCODE");
            t_paquete* paqueteRecibir = recibirPaquete(socketCliente);
            if (!paqueteRecibir) {
                log_error(logger, "Error recibiendo paquete en socket %d", socketCliente);
                exit(EXIT_FAILURE);
            }
            eliminarPaquete(paqueteRecibir);
            enviarOpcode(RESPUESTA_OK,socketCliente);
            break;
        }
        case TRUNCATE_FILE: {
            log_debug(logger,"Se recibio OPCODE");
            t_paquete* paquete = recibirPaquete(socketCliente);
            if (!paquete) {
                log_error(logger, "Error recibiendo paquete en socket %d", socketCliente);
                exit(EXIT_FAILURE);
            }
            eliminarPaquete(paquete);
            enviarOpcode(RESPUESTA_OK, socketCliente);
            break;
        }

        case TAG_FILE:{
            log_debug(logger,"Se recibio OPCODE");
            t_paquete* paquete = recibirPaquete(socketCliente);
            if (!paquete) {
                log_error(logger, "Error recibiendo paquete en socket %d", socketCliente);
                exit(EXIT_FAILURE);
            }
            eliminarPaquete(paquete);
            enviarOpcode(RESPUESTA_OK, socketCliente);
            break;
        }
        case COMMIT_FILE:{ //antes COMMIT_TAG
        log_debug(logger,"Se recibio OPCODE");
            t_paquete* paquete = recibirPaquete(socketCliente);
            eliminarPaquete(paquete);
            enviarOpcode(RESPUESTA_OK, socketCliente);
            break;
        }
        case WRITE_BLOCK: {
            log_debug(logger,"Se recibio OPCODE");
            t_paquete* paquete = recibirPaquete(socketCliente);
            eliminarPaquete(paquete);
            enviarOpcode(RESPUESTA_OK, socketCliente);
            break;
        }
        case READ_BLOCK:{
            log_debug(logger,"Se recibio OPCODE");
            t_paquete* paquete = recibirPaquete(socketCliente);
            eliminarPaquete(paquete);

            enviarOpcode(OBTENER_CONTENIDO_PAGINA,socketCliente);
            t_paquete*paquete2 = crearPaquete();
            char*contenido= "000000000000000000000000000000000000000";
            agregarStringAPaquete(paquete2,contenido);
            enviarPaquete(paquete2,socketCliente);
            eliminarPaquete(paquete2);
            log_debug(logger,"Se envio contenido <%s>",contenido);
            break;
        }
        case DELETE_FILE: { // DELETE_TAG en la consigna
            log_debug(logger,"Se recibio OPCODE");
            t_paquete* paquete = recibirPaquete(socketCliente);
            eliminarPaquete(paquete);
            enviarOpcode(RESPUESTA_OK, socketCliente);
            break;
        }
        default:
            break;
        }
    }
}

void incrementarWorkers(int workerId){
    pthread_mutex_lock(&mutex_cantidad_workers);
    cantidadWorkers++;
    log_info(logger," ##Se conecta el Worker <%d> - Cantidad de Workers: <%d>",workerId,cantidadWorkers);
    pthread_mutex_unlock(&mutex_cantidad_workers);
}

void decrementarWorkers(int workerId){
    pthread_mutex_lock(&mutex_cantidad_workers);
    cantidadWorkers--;
    log_info(logger,"##Se desconecta el Worker <%d> - Cantidad de Workers: <%d>",workerId,cantidadWorkers);
    pthread_mutex_unlock(&mutex_cantidad_workers);
}

void registrarWorker(int socket, int workerId) {
    pthread_mutex_lock(&mutexWorkers);
    
    t_worker* worker = malloc(sizeof(t_worker));
    worker->socket = socket;
    worker->workerId = workerId;
    
    list_add(listadoWorker, worker);
    
    pthread_mutex_unlock(&mutexWorkers);
}

// Obtener workerId y remover de la lista
int obtenerYRemoverWorker(int socket) {
    pthread_mutex_lock(&mutexWorkers);
    
    int workerId = -1;
    
    // Buscar en la lista
    for (int i = 0; i < list_size(listadoWorker); i++) {
        t_worker* worker = list_get(listadoWorker, i);
        if (worker->socket == socket) {
            workerId = worker->workerId;
            list_remove(listadoWorker, i);
            free(worker);
            break;
        }
    }
    
    pthread_mutex_unlock(&mutexWorkers);
    return workerId;
}