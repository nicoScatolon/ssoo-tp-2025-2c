#include "conexiones.h"

int iniciarConexion(char* path, int prioridad){ //Iniciar conexion con cliente (Master)
    char* puertoMaster = string_itoa(configQ->puertoMaster);
    int socketMaster = crearConexion(configQ->IPMaster,puertoMaster,logger);
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
    return socketMaster;
}

void* esperarRespuesta(void* socketClienteVoid){
    int socketMaster = (intptr_t) socketClienteVoid;
    opcode codigo = recibirOpcode(socketMaster);
    if (codigo < 0) {
        log_warning(logger, "Cliente desconectado en socket %d", socketMaster);
        close(socketMaster);
        return NULL;
        }
    while(1){
        opcode codigo = recibirOpcode(socketMaster);
        if (codigo < 0) {
            log_warning(logger, "Cliente desconectado en socket %d", socketMaster);
            close(socketMaster);
            return NULL;
        }
        switch(codigo){
            case FINALIZAR_QUERY_CONTROL:{
                t_paquete* paquete = recibirPaquete(socketMaster);
                int offset = 0;
                char * motivo = recibirStringDePaqueteConOffset(paquete,&offset);
                log_info(logger,"## Query finalizada - <Motivo> <%s>",motivo);
                free(motivo);
                //finalizarQueryControl();
                eliminarPaquete(paquete);
                break;
            }
            case INFORMACION_QUERY_CONTROL:{
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
                return NULL;
                break;
        }
    }
    return NULL;
}