#ifndef PLANIFICADORES_H
#define PLANIFICADORES_H
#include "globals.h"
#include "utils/globales.h"
#include "utils/logs.h"
#include <pthread.h>
#include "utils/paquete.h"

// ------------------- Planificadores (hilos) -------------------
void* planificadorFIFO();
void* planificadorPrioridad();
void* evaluarDesalojo();     
void* aging();            
// ------------------- Predicados / Sync -------------------
bool hayWorkerLibre(void);
bool hay_trabajo_para_planificar(void);
void despertar_planificador(void);

// ------------------- Helpers de colas/estados -------------------
worker* obtenerWorkerLibre(void);           // marca ocupado=true
query*  obtenerQuery(void);                 // saca 1ra de READY (FIFO)
void    cambioEstado(t_list_mutex* lista, query* elemento);
query*  obtenerQueryDeMenorPrioridad(void); // menor número = mayor prioridad
int solicitarDesalojoYObtenerPC(worker* w);

// ------------------- I/O con Worker -------------------
void enviarQueryAWorker(worker* workerElegido, char* path, int PC, int queryID);

// ------------------- Búsquedas utilitarias -------------------
queryControl* buscarQueryControlPorId(int idQuery);
worker* buscarWorkerPorQueryId(int idQuery);
int           obtenerPosicionQCPorId(int idBuscado);
int           obtenerPosicionWPorId(int idBuscado);
void          liberarWorker(worker* w);

#endif