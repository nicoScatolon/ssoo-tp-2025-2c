#include "configuracion.h"
void iniciarConfiguracionMaster(char*nombreConfig ,configMaster* configM){
    t_config* config = iniciarConfig(nombreConfig);
    *configM = agregarConfiguracionMaster(config);
}