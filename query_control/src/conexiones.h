#ifndef CONEXIONES_H
#define CONEXIONES_H

#include "configuracion.h"
#include "utils/paquete.h"
#include "utils/config.h"
#include "utils/globales.h"


void iniciarConexion(char* path, int prioridad);
void esperarRespuesta();
void finalizarQueryControl();
void manejar_sigint(int sig);
extern configQuery *configQ;
extern t_log* logger;

#endif
