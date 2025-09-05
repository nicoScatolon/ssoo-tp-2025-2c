#include "cleanup.h"
#include "config.h"
#include "logs.h"
void liberarModulo(t_log* logger,t_config *config){
    log_destroy(logger);
    config_destroy(config);
}
void liberarQuery(t_log*logger, t_config* configQ){
    liberarModulo(logger,configQ);
    // la idea es liberar mas cosas propias de cada modulo
}

void liberarWorker(t_log* logger, t_config* configW){
    liberarModulo(logger,configW);
    
}

void liberarMaster(t_log* logger, t_config* configM){
    liberarModulo(logger,configM);
    
}

void liberarStorage(t_log* logger, t_config*configS){
    liberarModulo(logger,configS);
}

