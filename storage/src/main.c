#include "main.h"

t_log* logger;
configStorage* configS;
configSuperBlock* configSB;

int main(int argc, char*argv[]){
    if (argc < 2){
        printf("No se pasaron los parametros necesarios, se necesita el nombre del archivo de configuracion\n");
        return EXIT_FAILURE;
    }
    char* nombreConfig = argv[1]; 
    
    configS = malloc(sizeof(configStorage));
    iniciarConfiguracionStorage(nombreConfig, configS);
    configSB = malloc(sizeof(configSuperBlock));
    iniciarConfiguracionSuperBlock(configSB);
    logger = iniciar_logger("storage", configS->logLevel);
    mostrarConfiguracionStorage(configS);
    levantarFileSystem();
    establecerConexionesStorage();
    pthread_exit(NULL);
}

void mostrarConfiguracionStorage(configStorage* config) {
    log_debug(logger, "========================================");
    log_debug(logger, "=== Configuración de Storage =========");
    log_debug(logger, "========================================");
    log_debug(logger, "PUERTO_ESCUCHA:        %d", config->puertoEscucha);
    log_debug(logger, "FRESH_START:           %s", config->freshStart ? "TRUE" : "FALSE");
    log_debug(logger, "PUNTO_MONTAJE:         %s", config->puntoMontaje);
    log_debug(logger, "RETARDO_OPERACION:     %d ms", config->retardoOperacion);
    log_debug(logger, "RETARDO_ACCESO_BLOQUE: %d ms", config->retardoAccesoBloque);

    log_debug(logger, "========================================");
}