#ifndef CONEXIONES_H
#define CONEXIONES_H

#include "configuracion.h"
#include "utils/paquete.h"
#include "utils/config.h"
#include "utils/globales.h"

void iniciarConexion(char* path, int prioridad);
void esperarRespuesta();

extern configQuery *configQ;
extern t_log* logger;

#endif
