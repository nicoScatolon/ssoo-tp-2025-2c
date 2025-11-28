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
            break;
        }
        case CREATE_FILE:{ // HECHO
            t_paquete* paqueteRecibir = recibirPaquete(socketCliente);
            if (!paqueteRecibir) {
                log_error(logger, "Error recibiendo paquete en socket %d", socketCliente);
                exit(EXIT_FAILURE);
            }

            int offset = 0;
            int idQuery = recibirIntDePaqueteconOffset(paqueteRecibir,&offset);
            char* file = recibirStringDePaqueteConOffset(paqueteRecibir,&offset);
            char* tag = recibirStringDePaqueteConOffset(paqueteRecibir,&offset);
            eliminarPaquete(paqueteRecibir);
            aplicarRetardoOperacion();
            if (crearFile(file, tag,0)) {
                log_info(logger, "##<%d> - File Creado <%s>:<%s>", idQuery, file, tag);
                enviarOpcode(RESPUESTA_OK, socketCliente);
            } else {
                enviarOpcode(RESPUESTA_ERROR, socketCliente);
            }
            free(file);
            free(tag);
            break;
        }
        case TRUNCATE_FILE: {
            t_paquete* paquete = recibirPaquete(socketCliente);
            if (!paquete) {
                log_error(logger, "Error recibiendo paquete en socket %d", socketCliente);
                exit(EXIT_FAILURE);
            }
            
            int offset = 0;
            int idQuery = recibirIntDePaqueteconOffset(paquete, &offset);
            char* file = recibirStringDePaqueteConOffset(paquete, &offset);
            char* tag = recibirStringDePaqueteConOffset(paquete, &offset);
            int nuevoTamanio = recibirIntDePaqueteconOffset(paquete, &offset);

            aplicarRetardoOperacion();

            if (!truncarArchivo(file, tag, nuevoTamanio, idQuery)) {
                enviarOpcode(RESPUESTA_ERROR, socketCliente);
            } else {
                enviarOpcode(RESPUESTA_OK, socketCliente);
            }

            free(file);
            free(tag);
            eliminarPaquete(paquete);
            break;
        }

        case TAG_FILE:{ //HECHO
            t_paquete* paquete = recibirPaquete(socketCliente);
            if (!paquete) {
                    log_error(logger, "Error recibiendo paquete en socket %d", socketCliente);
                    exit(EXIT_FAILURE);
                }
                int offset = 0;
                int idQuery = recibirIntDePaqueteconOffset(paquete,&offset);
                char* fileOrigen = recibirStringDePaqueteConOffset(paquete,&offset);
                char* tagOrigen = recibirStringDePaqueteConOffset(paquete,&offset);
                char* fileDestino= recibirStringDePaqueteConOffset(paquete,&offset);
                char* tagDestino = recibirStringDePaqueteConOffset(paquete,&offset);
                
                aplicarRetardoOperacion();
            
                if(!tagFile(fileOrigen, tagOrigen, fileDestino, tagDestino, idQuery)){
                    enviarOpcode(RESPUESTA_ERROR,socketCliente);
                } else{
                    log_info(logger, "##<%d> - Tag creado <%s>:<%s>", idQuery, fileDestino, tagDestino);
                    enviarOpcode(RESPUESTA_OK, socketCliente);
                }
                free(fileOrigen);
                free(tagOrigen);
                free(fileDestino);
                free(tagDestino);
                eliminarPaquete(paquete);

                break;
        }
        case COMMIT_FILE:{ // HECHO
            t_paquete* paquete = recibirPaquete(socketCliente);
            int offset = 0;
            int idQuery = recibirIntDePaqueteconOffset(paquete,&offset);
            char * file = recibirStringDePaqueteConOffset(paquete,&offset);
            char * tag = recibirStringDePaqueteConOffset(paquete,&offset);

            aplicarRetardoOperacion();
            hacerCommited(file, tag, idQuery);
            log_info(logger, "##<%d> - Commit de File:Tag <%s>:<%s>", idQuery, file, tag);
            enviarOpcode(RESPUESTA_OK, socketCliente);

            eliminarPaquete(paquete);
            free(file);
            free(tag);
            break;
        }
        case WRITE_BLOCK: {
            t_paquete* paquete = recibirPaquete(socketCliente);        
            int offset = 0;
            int idQuery = recibirIntDePaqueteconOffset(paquete, &offset);
            char* file = recibirStringDePaqueteConOffset(paquete, &offset);
            char* tag = recibirStringDePaqueteConOffset(paquete, &offset);
            int numeroBloque = recibirIntDePaqueteconOffset(paquete, &offset);
            char* contenido = recibirStringDePaqueteConOffset(paquete, &offset);

            aplicarRetardoOperacion();
            
            if (!escribirBloque(file, tag, numeroBloque, contenido, idQuery)) {
                enviarOpcode(RESPUESTA_ERROR, socketCliente);
            } else {
                enviarOpcode(RESPUESTA_OK, socketCliente);
            }

            free(file);
            free(tag);
            free(contenido);
            eliminarPaquete(paquete);
            break;
        }
       case FLUSH_FILE: {
    t_paquete* paqueteAviso = recibirPaquete(socketCliente);
    int offset = 0;
    int idQuery = recibirIntDePaqueteconOffset(paqueteAviso, &offset);
    char* file = recibirStringDePaqueteConOffset(paqueteAviso, &offset);
    char* tag = recibirStringDePaqueteConOffset(paqueteAviso, &offset);
    int cantidadBloques = recibirIntDePaqueteconOffset(paqueteAviso, &offset);
    
    log_debug(logger, "##<%d> - Iniciando FLUSH de <%s>:<%s> con <%d> bloques", 
              idQuery, file, tag, cantidadBloques);
    
    t_paquete* paqueteContenido = recibirPaquete(socketCliente);
    
    bool huboError = false;
    int i = 0;
    
    while (i < cantidadBloques && !huboError) {
        offset = 0;
        int numeroBloque = recibirIntDePaqueteconOffset(paqueteContenido, &offset);
        char* contenido = recibirStringDePaqueteConOffset(paqueteContenido, &offset);
        
        log_debug(logger, "##<%d> - FLUSH bloque <%d> con contenido <%s>", 
                  idQuery, numeroBloque, contenido);
        
        aplicarRetardoOperacion();
        
        if (!escribirBloque(file, tag, numeroBloque, contenido, idQuery)) {
            huboError = true;
        } else {
            log_debug(logger, "bloque numero <%d> escrito en FLUSH: <%s>", 
                      numeroBloque, contenido);
        }
        
        free(contenido);
        i++;
    }
    
    if (huboError) {
        enviarOpcode(RESPUESTA_ERROR, socketCliente);
    } else {
        enviarOpcode(RESPUESTA_OK, socketCliente);
    }
    
    free(file);
    free(tag);
    eliminarPaquete(paqueteAviso);
    eliminarPaquete(paqueteContenido);
    break;
}
        case READ_BLOCK:{ // HECHO
            t_paquete* paqueteRecibido = recibirPaquete(socketCliente);
            int offsetRecibido = 0;
            int idQuery = recibirIntDePaqueteconOffset(paqueteRecibido,&offsetRecibido);
            char * file = recibirStringDePaqueteConOffset(paqueteRecibido,&offsetRecibido);
            char * tag = recibirStringDePaqueteConOffset(paqueteRecibido,&offsetRecibido);
            int numeroBloque = recibirIntDePaqueteconOffset(paqueteRecibido,&offsetRecibido);
            
            char* contenido = lecturaBloque(file,tag,numeroBloque);
            aplicarRetardoOperacion();
            if(contenido == NULL){
                enviarOpcode(RESPUESTA_ERROR,socketCliente);
                break;
            }
            log_info(logger,"##<%d> - Bloque Lógico Leído <%s>:<%s> - Número de Bloque: <%d>",idQuery,file,tag,numeroBloque);
            
            enviarOpcode(OBTENER_CONTENIDO_PAGINA,socketCliente); //Agregarlo al hacer el merge
            t_paquete* paqueteEnviar = crearPaquete();
            //agregarIntAPaquete(paqueteEnviar,numeroBloque);
            agregarStringAPaquete(paqueteEnviar,contenido);
            enviarPaquete(paqueteEnviar, socketCliente);
        
            eliminarPaquete(paqueteRecibido);
            eliminarPaquete(paqueteEnviar);
            free(contenido);
            free(file);
            free(tag);
            break;
        }
        case DELETE_FILE: { //HECHO
            t_paquete* paquete = recibirPaquete(socketCliente);        
            int offset = 0;
            int idQuery = recibirIntDePaqueteconOffset(paquete, &offset);
            char* file = recibirStringDePaqueteConOffset(paquete, &offset);
            char* tag = recibirStringDePaqueteConOffset(paquete, &offset);

            aplicarRetardoOperacion();

            if (!eliminarTag(file, tag, idQuery)) {
                enviarOpcode(RESPUESTA_ERROR, socketCliente);
            } else {
                enviarOpcode(RESPUESTA_OK, socketCliente);
            }

            free(file);
            free(tag);
            eliminarPaquete(paquete);
            break;
        }
        default:
            break;
        }
    }
    return NULL;    
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