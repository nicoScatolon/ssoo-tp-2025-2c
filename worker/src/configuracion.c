#include "configuracion.h"
void iniciarConfiguracionWorker(char*nombreConfig ,configWorker* configWorker){
    t_config* config = iniciarConfig("worker",nombreConfig);
    *configW = agregarConfiguracionWorker(config);
}