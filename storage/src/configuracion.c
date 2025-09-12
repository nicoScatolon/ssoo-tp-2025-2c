#include "configuracion.h"
void iniciarConfiguracionStorage(char*nombreConfig ,configStorage* configS){
    t_config* config = iniciarConfig("storage",nombreConfig);
    *configS = agregarConfiguracionStorage(config);
}