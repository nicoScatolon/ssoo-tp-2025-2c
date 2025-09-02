#include "config.h"

t_config* iniciarConfig(char* path){
    t_config* config = config_create(path)
    if (config == NULL){
        printf("Error al leer el archivo de configuracion: %s\n",path);
        exit(EXIT_FAILURE);
    }
    return config;
}

configMaster cargarConfiguracionMaster(t_config* config){
    configMaster configMaster;
    configMaster.puertoEscucha = config_get_int_value(config,"PUERTO_ESCUCHA");
    configMaster.algoritmoPlanificacion = config_get_string_value(config,"ALGORITMO_PLANIFICACION");
    configMaster.tiempoAging=config_get_int_value(config,"TIEMPO_AGING");
    configMaster.logLevel=  config_get_string_value(config,"LOG_LEVEL");
    return configMaster;
}

configQuery cargarConfiguracionQuery(t_config* config){
    configQuery configQuery;
    configQuery.IPMaster = config_get_string_value(config,"IP_MASTER");
    configQuery.puertoMaster = config_get_int_value(config,"PUERTO_MASTER");
    configQuery.logLevel=  config_get_string_value(config,"LOG_LEVEL");
    return configQuery;
}

configWorker cargarConfiguracionWorker(t_config* config){
    configWorker configWorker;
    configWorker.IPMaster = config_get_string_value(config,"IP_MASTER");
    configWorker.puertoMaster = config_get_int_value(config,"PUERTO_MASTER");
    configWorker.IPStorage = config_get_string_value(config,"IP_STORAGE");
    configWorker.puertoStorage = config_get_int_value(config,"PUERTO_STORAGE");
    configWorker.tamMemoria = config_get_int_value(config,"TAM_MEMORIA");
    configWorker.retardoMemoria = config_get_int_value(config,"RETARDO_MEMORIA");
    configWorker.algoritmoReemplazo = config_get_string_value(config,"ALGORITMO_REEMPLAZO");
    configWorker.pathQueries = config_get_string_value(config,"PATH_QUERIES");
    configWorker.logLevel =  config_get_string_value(config,"LOG_LEVEL");
    return configWorker;
}

configStorage cargarConfiguracionStorage(t_config* config){
    configStorage configStorage;
    configStorage.puertoEscucha = config_get_int_value(config,"PUERTO_ESCUCHA");
    configStorage.aux= config_get_string_value(config,"FRESH_START");
    configStorage.freshStart = string_to_bool(config_get_string_value(config, "FRESH_START"));
    configStorage.puntoMontaje = config_get_string_value(config,"PUNTO_MONTAJE");
    configStorage.retardoOperacion= config_get_int_value(config,"RETARDO_OPERACION");
    configStorage.retardoAccesoBloque = config_get_int_value(config,"RETARDO_ACCESO_BLOQUE");
    configStorage.logLevel =  config_get_string_value(config,"LOG_LEVEL");
    return configStorage;
}

bool string_to_bool(char* str) {
    if (!str) return false;
    return (strcasecmp(str, "TRUE") == 0)
       
}