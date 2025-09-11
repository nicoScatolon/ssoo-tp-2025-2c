#ifndef PLANIFICADORES_H
#define PLANIFICADORES_H
#include "globals.h"
#include "utils/globales.h"
#include "utils/logs.h"
#include <pthread.h>


worker * obtenerWorkerLibre();
query* obtenerQuery(t_list_mutex* lista);
void* planificadorFIFO();
void cambioEstado(t_list_mutex* lista, query* elemento);
void despertar_planificador();
bool hay_trabajo_para_planificar();

#endif