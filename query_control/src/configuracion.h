
#ifndef CONFIGURACION_H
#define CONFIGURACION_H
#include "utils/config.h"
#include "utils/sockets.h"
#include "utils/logs.h"

extern configQuery* configQ;
extern t_log* logger;


void iniciarConfiguracionQueryControl(char*nombreConfig ,configQuery* configQuery);


#endif