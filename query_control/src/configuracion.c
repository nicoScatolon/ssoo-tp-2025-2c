#include "configuracion.h"
void iniciarConfiguracionQueryControl(char*nombreConfig ,configQuery*configQ){
    t_config* config = iniciarConfig("query_control",nombreConfig);
    *configQ = agregarConfiguracionQuery(config);
}