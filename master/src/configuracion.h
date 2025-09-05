
#ifndef CONFIGURACION_H
#define CONFIGURACION_H
#include "utils/config.h"
#include "utils/sockets.h"
#include "utils/logs.h"

extern configMaster* configM;
extern t_log* logger;

void iniciarConfiguracionMaster(char* nombreConfig, configMaster*configM);

#endif