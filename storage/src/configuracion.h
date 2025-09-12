#ifndef CONFIGURACION_H
#define CONFIGURACION_H
#include "utils/config.h"
#include "utils/sockets.h"
#include "utils/logs.h"

extern configStorage* configS;
extern t_log* logger;

//firmas de funciones aqui
void iniciarConfiguracionStorage(char*nombreConfig ,configStorage* configS);

#endif