#ifndef UTILS_CONFIG_H_
#define UTILS_CONFIG_H_

#include "../commons/common.h"
#include "commons/log.h"
#include "globales.h"

typedef struct 
{
    uint32_t puertoEscuchaQueryControl;
    uint32_t puertoEscuchaWorker;
    char* algoritmoPlanificacion;
    uint32_t tiempoAging;
    t_log_level logLevel;
} configMaster;

typedef struct{
    char* IPMaster;
    uint32_t puertoMaster;
    t_log_level logLevel;
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
    t_log_level logLevel;
    uint32_t FS_SIZE;
    uint32_t BLOCK_SIZE;
} configWorker;

typedef struct{
    uint32_t puertoEscucha;
    bool freshStart;
    char* puntoMontaje;
    uint32_t retardoOperacion;
    uint32_t retardoAccesoBloque;
    t_log_level logLevel;
} configStorage;

t_config* iniciarConfig(const char* modulo, const char* archivo);
configMaster agregarConfiguracionMaster(t_config* config);
configQuery agregarConfiguracionQuery(t_config* config);
configWorker agregarConfiguracionWorker(t_config* config);
configStorage agregarConfiguracionStorage(t_config*config);
bool string_to_bool(char* str);
t_log_level obtenerNivelLog(t_config* config);
void checkNULL(t_config* config) ;
#endif
