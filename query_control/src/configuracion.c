#include "configuracion.h"
void iniciarConfiguracionQueryControl(char*nombreConfig ,configQuery** configQ){
    t_config* config = iniciarConfig(nombreConfig);
    *configQ = agregarConfiguracionQuery(config);
}