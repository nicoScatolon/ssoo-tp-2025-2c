
#ifndef CONFIGURACION_H
#define CONFIGURACION_H
#include "utils/config.h"
#include "utils/sockets.h"
#include "globals.h"
#include "conexiones.h"
#include "memoria_interna.h"

extern configWorker* configW;

void inicializarCosas();
void iniciarConfiguracionWorker(char*nombreConfig ,configWorker* configWorker);

#endif

