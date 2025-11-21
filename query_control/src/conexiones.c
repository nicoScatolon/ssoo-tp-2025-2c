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
        int recibido = recv(socketMaster,&codigo,sizeof(opcode),0);
        if (recibido <= 0) {
            log_warning(logger, "Cliente desconectado en socket %d", socketMaster);
            close(socketMaster);
            //finalizarQueryControl();
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
                finalizarQueryControl();
                exit(EXIT_FAILURE);
                break;
            }
            case LECTURA_QUERY_CONTROL:{
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
// void manejar_sigint(int sig) {
//     write(STDOUT_FILENO, "\n[SIGINT] Desconectando del Master...\n", 39);
//     if (socketMaster > 0) {
//         enviarOpcode(DESCONEXION_QUERY_CONTROL, socketMaster);
//     }
//     usleep(1000);
//     log_debug(logger, "Desconectando del Master y saliendo...");
//     log_destroy(logger);
//     close(socketMaster);
//     exit(EXIT_SUCCESS);
// }

void manejar_sigint(int sig) {
    const char* msg = "\n\n*** SIGINT CAPTURADO ***\n";
    write(STDOUT_FILENO, msg, strlen(msg));
    
    write(STDOUT_FILENO, "Paso 1: Verificando socket...\n", 31);
    
    if (socketMaster > 0) {
        write(STDOUT_FILENO, "Paso 2: Enviando opcode...\n", 28);
        enviarOpcode(DESCONEXION_QUERY_CONTROL, socketMaster);
        
        write(STDOUT_FILENO, "Paso 3: Esperando envío...\n", 28);
        usleep(100000);
        
        write(STDOUT_FILENO, "Paso 4: Cerrando socket...\n", 28);
        close(socketMaster);
    }
    
    write(STDOUT_FILENO, "Paso 5: Cerrando logger...\n", 28);
    if (logger != NULL) {
        log_debug(logger, "Desconectando del Master y saliendo...");
        log_destroy(logger);
    }
    
    write(STDOUT_FILENO, "Paso 6: Saliendo...\n\n", 21);
    _exit(EXIT_SUCCESS);
}