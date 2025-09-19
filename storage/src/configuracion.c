#include "configuracion.h"
void iniciarConfiguracionStorage(char*nombreConfig ,configStorage* configS){
    t_config* config = iniciarConfig("storage",nombreConfig);
    *configS = agregarConfiguracionStorage(config);
}


void iniciarConfiguracionSuperBlock(configSuperBlock* configSB){
    t_config* config = iniciarConfigSuperBlock(configS->puntoMontaje);
    *configSB = agregarConfiguracionSuperBlock(config);
}