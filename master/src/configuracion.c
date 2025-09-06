#include "configuracion.h"
void iniciarConfiguracionMaster(char*nombreConfig ,configMaster* configM){
    t_config* config = iniciarConfig("master",nombreConfig);
    *configM = agregarConfiguracionMaster(config);
}