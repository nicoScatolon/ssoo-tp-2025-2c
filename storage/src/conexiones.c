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
        t_paquete* paquete = crearPaquete();
        agregarIntAPaquete(paquete,configSB->FS_SIZE);
        agregarIntAPaquete(paquete,configSB->BLOCK_SIZE);
        enviarPaquete(paquete,socketCliente);
        eliminarPaquete(paquete);
        break;
    }
    case CREATE_FILE:{
        t_paquete* paquete = recibirPaquete(socketCliente);
        if (!paquete) {
                log_error(logger, "Error recibiendo paquete en socket %d", socketCliente);
                break;
            }
            int offset = 0;
            char* file = recibirStringDePaqueteConOffset(paquete,&offset);
            char* tag = recibirStringDePaqueteConOffset(paquete,&offset);
            //crearFile(file,tag);
            free(file);
            free(tag);
            eliminarPaquete(paquete);
            break;
    }
    case TRUNCATE_FILE:{
        t_paquete* paquete = recibirPaquete(socketCliente);
        if (!paquete) {
                log_error(logger, "Error recibiendo paquete en socket %d", socketCliente);
                break;
            }
            int offset = 0;
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
            char* fileOrigen = recibirStringDePaqueteConOffset(paquete,&offset);
            char* tagOrigen = recibirStringDePaqueteConOffset(paquete,&offset);
            char* fileDestino= recibirStringDePaqueteConOffset(paquete,&offset);
            char* tagDestino = recibirStringDePaqueteConOffset(paquete,&offset);
            //Operaciones
            
            free(fileOrigen);
            free(tagOrigen);
            free(fileDestino);
            free(tagDestino);
            eliminarPaquete(paquete);
            break;
    }
    case COMMIT_FILE:{ //antes COMMIT_TAG
        break;
    }
    case WRITE_BLOCK:{
        break;
    }
    case READ_BLOCK:{
        break;
    }
    case DELETE_FILE:{//antes DELETE_TAG
        break;
    }
    default:
        break;
    }
    }
}