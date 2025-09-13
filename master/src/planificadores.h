#ifndef PLANIFICADORES_H
#define PLANIFICADORES_H
#include "globals.h"
#include "utils/globales.h"
#include "utils/logs.h"
#include <pthread.h>
#include "utils/paquete.h"

worker * obtenerWorkerLibre();
query* obtenerQuery(t_list_mutex* lista);
void* planificadorFIFO();
void cambioEstado(t_list_mutex* lista, query* elemento);
void despertar_planificador();
bool hay_trabajo_para_planificar();
void enviarQueryAWorker(worker* workerElegido,char* path,int PC,int queryID);
queryControl* buscarQueryControlPorId(int idQuery);
worker* buscarWorkerPorId(int idQuery);
int obtenerPosicionQCPorId(int idBuscado);
int obtenerPosicionWPorId(int idBuscado);
void liberarWorker(worker* workerA);
#endif