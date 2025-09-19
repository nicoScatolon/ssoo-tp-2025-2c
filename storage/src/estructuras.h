#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H

#include "configuracion.h"
#include "utils/config.h"
#include "utils/globales.h"
#include <sys/stat.h>
#include <sys/types.h>


extern configStorage *configS;
extern configSuperBlock *configSB;

extern t_log* logger;

void inicializarEstructuras();
void vaciarMemoria();

#endif