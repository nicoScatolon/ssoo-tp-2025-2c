#include "conexiones.h"

int socketMaster;
int socketStorage; //hacerlas globales
contexto_query_t* contexto = NULL;




void conexionConMaster(int ID) {
    char* puertoMaster = string_itoa(configW->puertoMaster);
    socketMaster = crearConexion(configW->IPMaster, puertoMaster, logger);
    comprobarSocket(socketMaster, "Worker", "Master", logger);
    log_debug(logger," ## Conexión al Master exitosa. IP: <%s>, Puerto: <%d>", configW->IPMaster,configW->puertoMaster);
    
    t_paquete* paquete = crearPaquete();
    enviarHandshake(socketMaster, WORKER);
    enviarOpcode(INICIAR_WORKER,socketMaster);
    agregarIntAPaquete(paquete,ID);
    enviarPaquete(paquete,socketMaster);

    eliminarPaquete(paquete);
    free(puertoMaster);
}


//esto deberia estar corriendo en un hilo aparte
void escucharMaster() {
    
    while (1) {
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
            case DESALOJO_QUERY_PLANIFICADOR:{
                sem_post(&sem_hayInterrupcion); 
                t_paquete* paquete = recibirPaquete(socketMaster);
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
                t_paquete* paquete = recibirPaquete(socketMaster);
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
                log_warning(logger, "Opcode desconocido recibido del Master: %d", codigo);
                break;
        }
    }
}


void conexionConStorage() {
    char* puertoStorage = string_itoa(configW->puertoStorage);
    socketStorage = crearConexion(configW->IPStorage, puertoStorage, logger);
    comprobarSocket(socketStorage, "Worker", "Storage", logger);
    log_debug(logger," ## Conexión al Storage exitosa. IP: <%s>, Puerto: <%d>", configW->IPStorage,configW->puertoStorage);
    
    enviarHandshake(socketStorage, WORKER);
    enviarOpcode(HANDSHAKE_STORAGE_WORKER,socketStorage);
    free(puertoStorage);
}

//esto deberia estar corriendo en un hilo aparte
void escucharStorage() {
    while (1) {
        opcode codigo;
        int recibido = recv(socketStorage,&codigo,sizeof(opcode),MSG_WAITALL);
        if (recibido <= 0) {
            log_warning(logger, "## Desconexión del Storage en socket <%d>", socketStorage);
            close(socketMaster);
            return;
        }
        switch (codigo) {
            case RESPUESTA_OK: { // NUEVO opcode
                log_debug(logger,"Se ejecuto la instruccion correctamente");
                break;
            }
            case RESPUESTA_ERROR:{
                t_paquete* paquete = recibirPaquete(socketStorage);
                if(!paquete){
                    log_error(logger, "Error recibiendo paquete de RESPUESTA_ERROR");
                    break;
                }
                int offset = 0;
                char * motivo = recibirStringDePaqueteConOffset(paquete,&offset);
                log_error(logger,"Error en la query, MOTIVO <%s>",motivo);
                
                //Enviar a master que hubo un error en la query
                enviarOpcode(FINALIZACION_QUERY,socketMaster);
                t_paquete* paquete2 = crearPaquete();
                if(!paquete2){
                    log_error(logger, "Error recibiendo paquete de RESPUESTA_ERROR");
                    break;
                }
                agregarIntAPaquete(paquete2,contexto->query_id);
                agregarIntAPaquete(paquete2,contexto->pc);
                enviarPaquete(paquete2,socketMaster);
                eliminarPaquete(paquete2);

                
                
                eliminarPaquete(paquete);
                free(motivo);
                // Se deberia notificar al master
                break;
            }
            case HANDSHAKE_STORAGE_WORKER:{
                t_paquete* paquete = recibirPaquete(socketStorage);
                int offset = 0;
                configW->FS_SIZE = recibirIntDePaqueteconOffset(paquete,&offset);
                configW->BLOCK_SIZE = recibirIntDePaqueteconOffset(paquete,&offset);
                eliminarPaquete(paquete);
                break;
            }
            default:
                log_warning(logger, "Opcode desconocido recibido del Storage: %d", codigo);
                break;
        }
    }
}