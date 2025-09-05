#include "cleanup.h"

void liberarQuery(t_log*logger, configQuery* configQ){
    liberarModulo(logger,configQ);
    // la idea es liberar mas cosas propias de cada modulo
}

void liberarWorker(t_log* logger, configWorker* configW){
    liberarModulo(logger,configW);
    
}

void liberarMaster(t_log* logger, configMaster* configM){
    liberarModulo(logger,configM);
    
}

void liberarStorage(t_log* logger, configStorage* configS){
    liberarModulo(logger,configS);
    
}

void liberarModulo(t_log* logger,t_config *config){
    log_destroy(logger);
    config_destroy(config);
}