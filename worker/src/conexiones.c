#include "conexiones.h"

int conexionConMaster(void) {
    char* puertoMaster = string_itoa(configW->puertoMaster);
    int socketMaster = crearConexion(configW->IPMaster, puertoMaster, logger);
    free(puertoMaster);

    comprobarSocket(socketMaster, "Worker", "Master", logger);

    enviarHandshake(socketMaster, WORKER);

    log_info(logger, "## Conexión al Master exitosa. IP: <%s>, Puerto: <%d>", configW->IPMaster, configW->puertoMaster);

    return socketMaster;
}

void escucharMaster(int socketMaster) {
    while (1) {
        opcode codigo = recibirOpcode(socketMaster);
        if (codigo <= 0) {
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

                log_info(logger, "## Master asignó nueva Query: <%s>", path);

                //

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
