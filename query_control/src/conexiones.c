#include "conexiones.h"
int socketMaster;
void iniciarConexion(char* path, int prioridad){ //Iniciar conexion con cliente (Master)
    char* puertoMaster = string_itoa(configQ->puertoMaster);
    socketMaster = crearConexion(configQ->IPMaster,puertoMaster,logger);
    comprobarSocket(socketMaster,"QueryControl","Master",logger);
    log_info(logger," ## Conexión al Master exitosa. IP: <%s>, Puerto: <%d>", configQ->IPMaster,configQ->puertoMaster);
    
    t_paquete* paquete = crearPaquete();
    enviarHandshake(socketMaster,QUERY_CONTROL);
    enviarOpcode(INICIAR_QUERY_CONTROL,socketMaster);
    
    agregarStringAPaquete(paquete,path);
    agregarIntAPaquete(paquete,prioridad);

    enviarPaquete(paquete,socketMaster);
    log_info(logger, " ## Solicitud de ejecución de Query: <%s>, prioridad: <%d>", path, prioridad);

    eliminarPaquete(paquete);
    free(puertoMaster);
}

void esperarRespuesta(){
    while(1){
        opcode codigo;
        int recibido = recv(socketMaster,&codigo,sizeof(opcode),MSG_WAITALL);
        if (recibido <= 0) {
            log_warning(logger, "Cliente desconectado en socket %d", socketMaster);
            close(socketMaster);
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

void finalizarQueryControl(){
    close(socketMaster);
    log_destroy(logger);
}