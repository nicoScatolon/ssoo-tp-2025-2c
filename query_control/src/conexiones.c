#include "conexiones.h"

void iniciarConexion(char* path, int prioridad){ //Iniciar conexion con cliente (Master)
    char* puertoMaster = string_itoa(configQ.puertoMaster);
    socketMaster = crearConexion(configQ.IPMaster,puertoMaster,logger);
    comprobarSocket(socketMaster,"QueryControl","Master");
    log_info(logger," ## Conexión al Master exitosa. IP: <%s>, Puerto: <%d>", configQ.IPMaster,config.puertoMaster);
    
    t_paquete* paquete = crearPaquete();
    agregarStringAPaquete(paquete,path);
    agregarIntAPaquete(paquete,prioridad);

    enviarHandshake(socketMaster,QUERY_CONTROL);
    enviarOpcode(INICIAR_QUERY_CONTROL,socketMaster);
    enviarPaquete(paquete,socketMaster);
    log_info(logger, " ## Solicitud de ejecución de Query: <%s>, prioridad: <%d>", path, prioridad);

    eliminarPaquete(paquete);
    free(puertoMaster);
}