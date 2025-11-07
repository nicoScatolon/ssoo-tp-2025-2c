#include "conexiones.h"

int socketMaster;
int socketMasterDesalojo;
int socketStorage;

contexto_query_t* contexto = NULL;

void conexionConMaster(int ID) {
    char* puertoMaster = string_itoa(configW->puertoMaster);
    socketMaster = crearConexion(configW->IPMaster, puertoMaster, logger);
    comprobarSocket(socketMaster, "Worker", "Master", logger);
    log_info(logger," ## Conexión al Master exitosa. IP: <%s>, Puerto: <%d>", configW->IPMaster,configW->puertoMaster);
    
    t_paquete* paquete = crearPaquete();
    enviarHandshake(socketMaster, WORKER);
    enviarOpcode(INICIAR_WORKER,socketMaster);
    agregarIntAPaquete(paquete,ID);
    enviarPaquete(paquete,socketMaster);

    eliminarPaquete(paquete);
    free(puertoMaster);
}
void conexionConMasterDesalojo() {
    char* puertoMasterDesalojo = string_itoa(configW->puertoMasterDesalojo);
    socketMasterDesalojo = crearConexion(configW->IPMaster, puertoMasterDesalojo, logger);
    comprobarSocket(socketMasterDesalojo, "Worker", "Master_Desalojo", logger);
    log_debug(logger," ## Conexión al Master_Desalojo exitosa. IP: <%s>, Puerto: <%d>", configW->IPMaster,configW->puertoMasterDesalojo);
    
    enviarHandshake(socketMasterDesalojo, WORKER);
    free(puertoMasterDesalojo);
}

void conexionConStorage() {
    char* puertoStorage = string_itoa(configW->puertoStorage);
    socketStorage = crearConexion(configW->IPStorage, puertoStorage, logger);
    comprobarSocket(socketStorage, "Worker", "Storage", logger);
    log_info(logger," ## Conexión al Storage exitosa. IP: <%s>, Puerto: <%d>", configW->IPStorage,configW->puertoStorage);
    
    enviarHandshake(socketStorage, WORKER);
    enviarOpcode(HANDSHAKE_STORAGE_WORKER,socketStorage);
    free(puertoStorage);
}

void escucharMaster() {
    while(1){
        opcode codigo;
        int recibido = recv(socketMaster,&codigo,sizeof(opcode),MSG_WAITALL);
        if (recibido <= 0) {
            log_warning(logger, "## Desconexión del Master en socket <%d>", socketMaster);
            close(socketMaster);
            return;
        }
        switch (codigo) {
            case NUEVA_QUERY: { // NUEVO opcode
                t_paquete* paquete = recibirPaquete(socketMaster);
                if (!paquete) {
                    log_error(logger, "Error recibiendo paquete de NUEVA_QUERY");
                    break;
                }

                int offset = 0;
                char* path = recibirStringDePaqueteConOffset(paquete, &offset);
                int idQuery = recibirIntDePaqueteconOffset(paquete,&offset);
                int pc = recibirIntDePaqueteconOffset(paquete,&offset);
                log_info(logger, "Recepción de Query: ## Query <%d>: Se recibe la Query. El path de operaciones es: <%s>",idQuery,path);

                memset(contexto, 0, sizeof(contexto_query_t));
                contexto = cargarQuery(path, idQuery, pc);

                
                ejecutarQuery(contexto); //hay que lanzar un hilo para esto.

                liberarContextoQuery(contexto);
                free(path);
                eliminarPaquete(paquete);
                break;
            }
            default:
                log_warning(logger, "Opcode desconocido recibido del Master: %d", codigo);
                break;
        }
    }
}
void* escucharDesalojo() {
    while (1) {
        opcode codigo;
        int recibido = recv(socketMasterDesalojo,&codigo,sizeof(opcode),MSG_WAITALL);
        if (recibido <= 0) {
            log_warning(logger, "## Desconexión del MasterDesalojo en socket <%d>", socketMasterDesalojo);
            close(socketMasterDesalojo);
            pthread_exit(NULL);
        }

        switch (codigo) {
            case DESALOJO_QUERY_PLANIFICADOR:{
                sem_post(&sem_hayInterrupcion); 
                t_paquete* paquete = recibirPaquete(socketMasterDesalojo);
                if(!paquete){
                    log_error(logger, "Error recibiendo paquete de DESALOJO_QUERY_PLANIFICADOR");
                    break;
                }
                int offset = 0;
                int idQuery = recibirIntDePaqueteconOffset(paquete,&offset);
                log_info(logger,"Desalojo de Query: ## Query <%d>: Desalojada por pedido del Master",idQuery);
            
                //ejecutar flush y enviar Id y PC actualizado (creo que conviene hacer todo esto en el query interpreter)
                desalojarQuery(idQuery, DESALOJO_QUERY_PLANIFICADOR);
                
                eliminarPaquete(paquete);
                break;
            }
            case DESALOJO_QUERY_DESCONEXION:{
                sem_post(&sem_hayInterrupcion); 
                t_paquete* paquete = recibirPaquete(socketMasterDesalojo);
                if(!paquete){
                    log_error(logger, "Error recibiendo paquete de DESALOJO_QUERY_DESCONEXION");
                    break;
                }
                int offset = 0;
                int idQuery = recibirIntDePaqueteconOffset(paquete,&offset);
                log_info(logger,"Desalojo de Query: ## Query <%d>: Desalojada por desconexión del cliente",idQuery);
            
                //ejecutar flush y enviar Id y PC actualizado (creo que conviene hacer todo esto en el query interpreter)
                desalojarQuery(idQuery, DESALOJO_QUERY_DESCONEXION);
                
                eliminarPaquete(paquete);
                break;
            }
            default:
                log_warning(logger, "Opcode desconocido recibido del MasterDesalojo: %d", codigo);
                break;
        }
    }
    return NULL;
}

int escucharStorage() {
    opcode codigo;
    int recibido = recv(socketStorage,&codigo,sizeof(opcode),MSG_WAITALL);
    if (recibido < 0) {
        log_error(logger, "## Desconexión del Storage en socket <%d>", socketStorage);
        close(socketStorage); 
        return -1;
    }
    switch (codigo) {
        case RESPUESTA_OK: { 
            log_debug(logger,"Se ejecuto la instruccion correctamente");
            return 0; //no hay error
            break;
        }
        case RESPUESTA_ERROR:{
            return -1; //hay error
            break;
        }
        case HANDSHAKE_STORAGE_WORKER:{
            t_paquete* paquete = recibirPaquete(socketStorage);
            int offset = 0;
            configW->FS_SIZE = recibirIntDePaqueteconOffset(paquete,&offset);
            configW->BLOCK_SIZE = recibirIntDePaqueteconOffset(paquete,&offset);
            eliminarPaquete(paquete);
            return 0;
            break;
        }
        default:
            log_warning(logger, "Opcode desconocido recibido del Storage: %d", codigo);
            return 0;
            break;
    }
}

char* escucharStorageContenidoPagina(){
    opcode codigo;
    int recibido = recv(socketStorage,&codigo,sizeof(opcode),MSG_WAITALL);
    if (recibido <= 0) {
        log_error(logger, "## Desconexión del Storage en socket <%d>", socketStorage);
        close(socketStorage);
        return NULL;
    }
    switch (codigo) {
        case RESPUESTA_ERROR:{
            return NULL;
            break;
        }
        case OBTENER_CONTENIDO_PAGINA:{
            t_paquete* paquete = recibirPaquete(socketStorage);
            if(!paquete){
                log_error(logger, "Error recibiendo paquete de OBTENER_CONTENIDO_PAGINA");
                return NULL;
            }
            int offset = 0;
            char* contenido = recibirStringDePaqueteConOffset(paquete,&offset);
                
            eliminarPaquete(paquete);
            return contenido;
        }
        default:
            log_error(logger, "Opcode desconocido recibido del Storage: %d", codigo);
            return NULL;
    }
}