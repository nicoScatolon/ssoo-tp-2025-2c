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
            log_warning(logger, "Cliente desconectado en socket %d", socketCliente);
            close(socketCliente);
            return NULL;
        }
    switch (codigo)
    {
    case HANDSHAKE_STORAGE_WORKER:{
        t_paquete*paqueteRecibir =recibirPaquete(socketCliente);
        int offset = 0;
        int workerId = recibirIntDePaqueteconOffset(paqueteRecibir,&offset);

        incrementarWorkers(workerId);
        t_paquete* paqueteEnviar = crearPaquete();
        agregarIntAPaquete(paqueteEnviar,configSB->FS_SIZE);
        agregarIntAPaquete(paqueteEnviar,configSB->BLOCK_SIZE);
        enviarPaquete(paqueteEnviar,socketCliente);
        eliminarPaquete(paqueteEnviar);
        eliminarPaquete(paqueteRecibir);
        break;
    }
    case CREATE_FILE:{
        t_paquete* paqueteRecibir = recibirPaquete(socketCliente);
        if (!paqueteRecibir) {
                log_error(logger, "Error recibiendo paquete en socket %d", socketCliente);
                break;
            }
            int offset = 0;
            int idQuery = recibirIntDePaqueteconOffset(paqueteRecibir,&offset);
            char* file = recibirStringDePaqueteConOffset(paqueteRecibir,&offset);
            char* tag = recibirStringDePaqueteConOffset(paqueteRecibir,&offset);
            crearFile(file,tag);
            aplicarRetardoOperacion();
            log_info(logger,"##<%d> - File Creado <%s>:<%s>",idQuery,file,tag);
            free(file);
            free(tag);
            eliminarPaquete(paqueteRecibir);
            enviarOpcode(RESPUESTA_OK,socketCliente);
            break;
    }
    case TRUNCATE_FILE:{
        t_paquete* paquete = recibirPaquete(socketCliente);
        if (!paquete) {
                log_error(logger, "Error recibiendo paquete en socket %d", socketCliente);
                break;
            }
            int offset = 0;
            int idQuery = recibirIntDePaqueteconOffset(paquete,&offset);
            char* file = recibirStringDePaqueteConOffset(paquete,&offset);
            char* tag = recibirStringDePaqueteConOffset(paquete,&offset);
            int nuevoTamanio = recibirIntDePaqueteconOffset(paquete,&offset);
            //Operaciones
            
            free(file);
            free(tag);
            eliminarPaquete(paquete);
            break;
    }

    case TAG_FILE:{
        t_paquete* paquete = recibirPaquete(socketCliente);
        if (!paquete) {
                log_error(logger, "Error recibiendo paquete en socket %d", socketCliente);
                break;
            }
            int offset = 0;
            int idQuery = recibirIntDePaqueteconOffset(paquete,&offset);

            char* fileOrigen = recibirStringDePaqueteConOffset(paquete,&offset);
            char* tagOrigen = recibirStringDePaqueteConOffset(paquete,&offset);
            char* fileDestino= recibirStringDePaqueteConOffset(paquete,&offset);
            char* tagDestino = recibirStringDePaqueteConOffset(paquete,&offset);
            
        
            if(!tagFile(fileOrigen, tagOrigen, fileDestino, tagDestino, idQuery)){
                aplicarRetardoOperacion();
                enviarOpcode(RESPUESTA_ERROR,socketCliente);
            }

            aplicarRetardoOperacion();
            log_info(logger,"");
            enviarOpcode(RESPUESTA_OK, socketCliente);
            
            free(fileOrigen);
            free(tagOrigen);
            free(fileDestino);
            free(tagDestino);
            eliminarPaquete(paquete);

            break;
    }
    case COMMIT_FILE:{ //antes COMMIT_TAG
        t_paquete* paquete = recibirPaquete(socketCliente);
        int offset = 0;
        int idQuery = recibirIntDePaqueteconOffset(paquete,&offset);
        char * file = recibirStringDePaqueteConOffset(paquete,&offset);
        char * tag = recibirStringDePaqueteConOffset(paquete,&offset);
        eliminarPaquete(paquete);
        free(file);
        free(tag);
        break;
    }
    case WRITE_BLOCK:{
        t_paquete* paquete = recibirPaquete(socketCliente);
        int offset = 0;
        int idQuery = recibirIntDePaqueteconOffset(paquete,&offset);
        char * file = recibirStringDePaqueteConOffset(paquete,&offset);
        char * tag = recibirStringDePaqueteConOffset(paquete,&offset);


        eliminarPaquete(paquete);
        free(file);
        free(tag);
        break;
    }
    case READ_BLOCK:{
        t_paquete* paquete = recibirPaquete(socketCliente);
        int offset = 0;
        int idQuery = recibirIntDePaqueteconOffset(paquete,&offset);
        int numeroBloque = recibirIntDePaqueteconOffset(paquete,&offset);
        char * file = recibirStringDePaqueteConOffset(paquete,&offset);
        char * tag = recibirStringDePaqueteConOffset(paquete,&offset);
        
        char* contenido = lecturaBloque(file,tag,numeroBloque);
        aplicarRetardoOperacion();
        if(contenido == NULL){
            enviarOpcode(RESPUESTA_ERROR,socketCliente);
        }
        log_info(logger,"##<%d> - Bloque Lógico Leído <%s>:<%s> - Número de Bloque: <%d>",idQuery,file,tag,numeroBloque);
        enviarOpcode(RESPUESTA_OK,socketCliente);
        eliminarPaquete(paquete);
        free(file);
        free(tag);
        break;
    }
    case DELETE_FILE:{//antes DELETE_TAG
        t_paquete* paquete = recibirPaquete(socketCliente);
        int offset = 0;
        int idQuery = recibirIntDePaqueteconOffset(paquete,&offset);
        char * file = recibirStringDePaqueteConOffset(paquete,&offset);
        char * tag = recibirStringDePaqueteConOffset(paquete,&offset);
        
        eliminarPaquete(paquete);
        free(file);
        free(tag);
        break;
    }
    case DESCONEXION_WORKER:{
        t_paquete *paquete = recibirPaquete(socketCliente);
        int offset = 0;
        int workerId = recibirIntDePaqueteconOffset(paquete,&offset);
        decrementarWorkers(workerId);
        close(socketCliente);
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