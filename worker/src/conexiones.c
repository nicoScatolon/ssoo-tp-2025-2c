#include "conexiones.h"

int socketMaster;
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

                free(path);
                eliminarPaquete(paquete);
                break;
            }
            case DESALOJO_QUERY_PLANIFICADOR:{
                t_paquete* paquete = recibirPaquete(socketMaster);
                if(!paquete){
                    log_error(logger, "Error recibiendo paquete de DESALOJO_QUERY_PLANIFICADOR");
                    break;
                }
                int offset = 0;
                int idQuery = recibirIntDePaqueteconOffset(paquete,&offset);
                log_info(logger,"Desalojo de Query: ## Query <%d>: Desalojada por pedido del Master",idQuery);
                //enviarId y PC actualizado
                eliminarPaquete(paquete);
                break;
            }
            default:
                log_warning(logger, "Opcode desconocido recibido del Master: %d", codigo);
                break;
        }
    }
}
void esperarRespuesta(){
    while(1){
        opcode codigo;
        int recibido = recv(socketMaster,&codigo,sizeof(opcode),MSG_WAITALL);
        if (recibido <= 0) {
            log_warning(logger, "Cliente desconectado en socket %d", socketMaster);
            break;
        }
        switch(codigo){
            case FINALIZACION_QUERY:{
                t_paquete* paquete = recibirPaquete(socketMaster);
                int offset = 0;
                char * motivo = recibirStringDePaqueteConOffset(paquete,&offset);
                log_info(logger,"## Query finalizada - <Motivo> <%s>",motivo);
                free(motivo);
                eliminarPaquete(paquete);
                //finalizarQueryControl();
                break;
            }
            case LECTURA_QUERY_CONTROL:{
                log_debug(logger,"Lectura en queryControl");
                t_paquete* paquete = recibirPaquete(socketMaster);
                int offset = 0;
                char * file = recibirStringDePaqueteConOffset(paquete,&offset);
                char * tag = recibirStringDePaqueteConOffset(paquete,&offset);
                char * lectura = recibirStringDePaqueteConOffset(paquete,&offset);
                log_info(logger,"## Lectura realizada: Archivo <%s:%s>, contenido: <%s>",file,tag,lectura);
                free(lectura);
                free(file);
                free(tag);
                eliminarPaquete(paquete);
                break;
            }
            default:
                log_warning(logger, " ## Respuesta desconocida de Master (opcode=%d)", codigo);
                break;
        }
    }
}