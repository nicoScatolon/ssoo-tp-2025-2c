#include "config.h"

t_config* iniciarConfig(const char* modulo, const char* archivo) {
    char pathConcatenado[512];  
    snprintf(
        pathConcatenado,
        sizeof(pathConcatenado),
        "/home/utnso/tp-2025-2c-Here-we-go-again/%s/configs/%s.config",
        modulo,
        archivo
    );

    t_config* config = config_create(pathConcatenado);
    if (config == NULL) {
        printf("Error al leer el archivo de configuracion: %s\n", pathConcatenado);
        exit(EXIT_FAILURE);
    }
    return config;
}

configMaster agregarConfiguracionMaster(t_config* config){
    configMaster configMaster;
    configMaster.puertoEscucha = config_get_int_value(config,"PUERTO_ESCUCHA");
    configMaster.algoritmoPlanificacion = config_get_string_value(config,"ALGORITMO_PLANIFICACION");
    configMaster.tiempoAging=config_get_int_value(config,"TIEMPO_AGING");
    configMaster.logLevel= obtenerNivelLog(config);
    return configMaster;
}

configQuery agregarConfiguracionQuery(t_config* config){
    configQuery configQuery;
    configQuery.IPMaster = config_get_string_value(config,"IP_MASTER");
    configQuery.puertoMaster = config_get_int_value(config,"PUERTO_MASTER");
    configQuery.logLevel=  obtenerNivelLog(config);
    return configQuery;
}

configWorker agregarConfiguracionWorker(t_config* config){
    configWorker configWorker;
    configWorker.IPMaster = config_get_string_value(config,"IP_MASTER");
    configWorker.puertoMaster = config_get_int_value(config,"PUERTO_MASTER");
    configWorker.IPStorage = config_get_string_value(config,"IP_STORAGE");
    configWorker.puertoStorage = config_get_int_value(config,"PUERTO_STORAGE");
    configWorker.tamMemoria = config_get_int_value(config,"TAM_MEMORIA");
    configWorker.retardoMemoria = config_get_int_value(config,"RETARDO_MEMORIA");
    configWorker.algoritmoReemplazo = config_get_string_value(config,"ALGORITMO_REEMPLAZO");
    configWorker.pathQueries = config_get_string_value(config,"PATH_QUERIES");
    configWorker.logLevel =  obtenerNivelLog(config);
    return configWorker;
}

configStorage agregarConfiguracionStorage(t_config* config){
    configStorage configStorage;
    configStorage.puertoEscucha = config_get_int_value(config,"PUERTO_ESCUCHA");
    configStorage.freshStart = string_to_bool(config_get_string_value(config, "FRESH_START"));
    configStorage.puntoMontaje = config_get_string_value(config,"PUNTO_MONTAJE");
    configStorage.retardoOperacion= config_get_int_value(config,"RETARDO_OPERACION");
    configStorage.retardoAccesoBloque = config_get_int_value(config,"RETARDO_ACCESO_BLOQUE");
    configStorage.logLevel =  obtenerNivelLog(config);
    return configStorage;
}

bool string_to_bool(char* str) {
    if (!str) return false;
    return (strcasecmp(str, "false") == 0);
}

t_log_level obtenerNivelLog(t_config* config) {
    checkNULL(config);
    char* str = config_get_string_value(config, "LOG_LEVEL");
    return log_level_from_string(str);
}

void checkNULL(t_config* config) {
    if (config != NULL) return;
    fprintf(stderr, "ERROR: config es NULL\n");
    exit(EXIT_FAILURE);
}
