#include "conexiones.h"

int socketMaster;
int socketMasterDesalojo;
int socketStorage;

contexto_query_t* contexto = NULL;
pthread_mutex_t mutex_socket_master;
void conexionConMaster(int workerId) {
    char* puertoMaster = string_itoa(configW->puertoMaster);
    socketMaster = crearConexion(configW->IPMaster, puertoMaster, logger);
    comprobarSocket(socketMaster, "Worker", "Master", logger);
    log_info(logger," ## Conexión al Master exitosa. IP: <%s>, Puerto: <%d>", configW->IPMaster,configW->puertoMaster);
    
    t_paquete* paquete = crearPaquete();
    enviarHandshake(socketMaster, WORKER);
    enviarOpcode(INICIAR_WORKER,socketMaster);
    agregarIntAPaquete(paquete,workerId);
    enviarPaquete(paquete,socketMaster);

    eliminarPaquete(paquete);
    free(puertoMaster);

    // pthread_t hilo_query;
    // pthread_create(&hilo_query,NULL,escucharMaster,NULL);
    // pthread_detach(hilo_query);
}

void conexionConMasterDesalojo(int workerId) {
    char* puertoMasterDesalojo = string_itoa(configW->puertoMasterDesalojo);
    socketMasterDesalojo = crearConexion(configW->IPMaster, puertoMasterDesalojo, logger);
    comprobarSocket(socketMasterDesalojo, "Worker", "Master_Desalojo", logger);
    log_info(logger," ## Conexión al Master_Desalojo exitosa. IP: <%s>, Puerto: <%d>", configW->IPMaster,configW->puertoMasterDesalojo);
    
    t_paquete* paquete = crearPaquete();
    enviarHandshake(socketMasterDesalojo, WORKER_DESALOJO);
    enviarOpcode(CONEXION_DESALOJO,socketMasterDesalojo); 
    agregarIntAPaquete(paquete,workerId);
    enviarPaquete(paquete,socketMasterDesalojo); //No estaba

    eliminarPaquete(paquete);
    free(puertoMasterDesalojo);
    
    pthread_t hilo_desalojo;
    pthread_create(&hilo_desalojo,NULL,escucharDesalojo,NULL);
    pthread_detach(hilo_desalojo);
}

void conexionConStorage(int workerId) {
    char* puertoStorage = string_itoa(configW->puertoStorage);
    socketStorage = crearConexion(configW->IPStorage, puertoStorage, logger);
    comprobarSocket(socketStorage, "Worker", "Storage", logger);
    log_info(logger," ## Conexión al Storage exitosa. IP: <%s>, Puerto: <%d>", configW->IPStorage,configW->puertoStorage);
    
    enviarHandshake(socketStorage, WORKER);
    enviarOpcode(HANDSHAKE_STORAGE_WORKER,socketStorage);
    
    t_paquete* paquete = crearPaquete();
    agregarIntAPaquete(paquete,workerId);
    enviarPaquete(paquete,socketStorage);
    eliminarPaquete(paquete);
    
    escucharStorage();
    free(puertoStorage);
}

void escucharMaster() {
    while(1) {
        opcode codigo;
        
        // ✅ Bloquear antes de leer
        pthread_mutex_lock(&mutex_socket_master);
        int recibido = recv(socketMaster, &codigo, sizeof(opcode), MSG_WAITALL);
        
        if (recibido <= 0) {
            pthread_mutex_unlock(&mutex_socket_master);
            log_warning(logger, "## Desconexión del Master en socket <%d>", socketMaster);
            close(socketMaster);
            exit(EXIT_FAILURE);
        }
        
        switch (codigo) {
            case NUEVA_QUERY: {
                t_paquete* paquete = recibirPaquete(socketMaster);
                pthread_mutex_unlock(&mutex_socket_master);  // Desbloquear después de leer
                
                if (!paquete) {
                    log_error(logger, "Error recibiendo paquete de NUEVA_QUERY");
                    exit(EXIT_FAILURE);
                }

                int offset = 0;
                char* path = recibirStringDePaqueteConOffset(paquete, &offset);
                int idQuery = recibirIntDePaqueteconOffset(paquete, &offset);
                int pc = recibirIntDePaqueteconOffset(paquete, &offset);
                
                log_info(logger, "Recepción de Query: ## Query <%d>: Se recibe la Query. El path de operaciones es: <%s>", idQuery, path);

                contexto = cargarQuery(path, idQuery, pc); 

                if (!contexto) {
                    log_error(logger, "Error al cargar query %d", idQuery);
                    free(path);
                    eliminarPaquete(paquete);
                    exit(EXIT_FAILURE);
                }
                
                ejecutarQuery(contexto);
                liberarContextoQuery(contexto);
                eliminarPaquete(paquete);
                break;
            }
            default:
                pthread_mutex_unlock(&mutex_socket_master);
                log_warning(logger, "Opcode desconocido recibido del Master: %d", codigo);
                break;
        }
    }
}
void* escucharDesalojo() {
    log_debug(logger,"Hilo corriendo");
    while (1) {
        opcode codigo;
        int recibido = recv(socketMasterDesalojo,&codigo,sizeof(opcode),MSG_WAITALL);
        log_debug(logger,"Opcode recibido <%d>",recibido);
        if (recibido < 0) {
            log_warning(logger, "## Desconexión del MasterDesalojo en socket <%d>", socketMasterDesalojo);
            close(socketMasterDesalojo);
            pthread_exit(NULL);
        }
        log_warning(logger,"Recibido codigo <%d> en hilo desalojo", codigo);
        
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
            
                //ejecuta el flush y enviar Id y PC actualizado
                desalojarQuery(idQuery, codigo);
                eliminarPaquete(paquete);
                
                break;
            }
            case DESALOJO_QUERY_DESCONEXION:{
                sem_post(&sem_hayInterrupcion); 
                sem_wait(&sem_interrupcionAtendida);
                log_debug(logger,"Notificando interrupcion");
                log_debug(logger,"socketMasterDesalojo <%d>",socketMasterDesalojo);
                log_debug(logger,"socketMaster <%d>",socketMaster);
                t_paquete* paquete = recibirPaquete(socketMasterDesalojo);
                if(!paquete){
                    log_error(logger, "Error recibiendo paquete de DESALOJO_QUERY_DESCONEXION");
                    exit(EXIT_FAILURE);
                }
                int offset = 0;
                int idQuery = recibirIntDePaqueteconOffset(paquete,&offset);
                log_info(logger,"Desalojo de Query: ## Query <%d>: Desalojada por desconexión del cliente",idQuery);
            
                eliminarPaquete(paquete);
                desalojarQuery(idQuery, DESALOJO_QUERY_DESCONEXION);
                //esta bien que aca tambièn se ejecute el flush??
                
                break;
            }
            default:
                log_warning(logger, "Opcode desconocido recibido del MasterDesalojo: %d", codigo);
                exit(EXIT_FAILURE);
        }
    }
    return NULL;
}


int escucharStorage() {
    opcode codigo;
    int recibido = recv(socketStorage,&codigo,sizeof(opcode),MSG_WAITALL);
    if (recibido < 0) {
        log_error(logger, "## Desconexión del Storage en socket <%d> - Recibido: %d", socketStorage, recibido);
        close(socketStorage); 
        return -1;
    }
    switch (codigo) {
        case RESPUESTA_OK: { 
            //log_debug(logger,"Se ejecuto la instruccion correctamente");
            return 0; //no hay error
            break;
        }
        case RESPUESTA_ERROR:{
            return -1; //hay error
            break;
        }
        case HANDSHAKE_STORAGE_WORKER:{
            // log_debug(logger,"entra en HANDSHAKE_STORAGE_WORKER");
            t_paquete* paquete = recibirPaquete(socketStorage);
            int offset = 0;
            configW->FS_SIZE = recibirIntDePaqueteconOffset(paquete,&offset);
            configW->BLOCK_SIZE = recibirIntDePaqueteconOffset(paquete,&offset);

            log_debug(logger,"Handshake con Storage completado. FS_SIZE: %d, BLOCK_SIZE: %d",configW->FS_SIZE,configW->BLOCK_SIZE);
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
            log_warning(logger, "Error recibido del Storage al obtener contenido de página");
            return NULL;
        }
        case OBTENER_CONTENIDO_PAGINA:{
            t_paquete* paquete = recibirPaquete(socketStorage);
            if(!paquete){
                log_error(logger, "Error recibiendo paquete de OBTENER_CONTENIDO_PAGINA");
                return NULL;
            }
            int offset = 0;
            char* contenido = recibirStringDePaqueteConOffset(paquete,&offset);
            log_debug(logger,"contenido <%s>",contenido);
            eliminarPaquete(paquete);
            return contenido;
        }
        default:
            log_error(logger, "Opcode desconocido recibido del Storage: %d", codigo);
            return NULL;
    }
}