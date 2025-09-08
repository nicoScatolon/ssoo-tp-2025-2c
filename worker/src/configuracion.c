#include "configuracion.h"
void iniciarConfiguracionWorker(char*nombreConfig ,configWorker* configWorker){
    t_config* config = iniciarConfig("woker",nombreConfig);
    *configW = agregarConfiguracionQuery(config);
}