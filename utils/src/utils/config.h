#ifndef UTILS_CONFIG_H_
#define UTILS_CONFIG_H_

#include "../commons/common.h"
#include "commons/log.h"

typedef struct 
{
    uint32_t puertoEscucha;
    char* algoritmoPlanificacion;
    uint32_t tiempoAging;
    char* logLevel;
} configMaster;

typedef struct{
    char* IPMaster;
    uint32_t puertoMaster;
    char* logLevel;
} configQuery;

typedef struct{
    char* IPMaster;
    uint32_t puertoMaster;
    char* IPStorage;
    uint32_t puertoStorage;
    uint32_t tamMemoria;
    uint32_t retardoMemoria;
    char* algoritmoReemplazo;
    char* pathQueries;
    char* logLevel;
} configWorker;

typedef struct{
    uint32_t puertoEscucha;
    bool freshStart;
    char* puntoMontaje;
    uint32_t retardoOperacion;
    uint32_t retardoAccesoBloque;
    char* logLevel;
} configStorage;

t_config* iniciarConfig(char* path)
configMaster cargarConfiguracionMaster(t_config* config);
configQuery cargarConfiguracionQuery(t_config* config);
configWorker cargarConfiguracionWorker(t_config* config);
configStorage cargarConfiguracionStorage(t_config*config);
bool string_to_bool(char* str)
#endif
