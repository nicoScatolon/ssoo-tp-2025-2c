
#ifndef CONFIGURACION_H
#define CONFIGURACION_H
#include "utils/config.h"
#include "utils/sockets.h"

extern configWorker* configW;

void iniciarConfiguracionWorker(char*nombreConfig ,configWorker* configWorker);

#endif