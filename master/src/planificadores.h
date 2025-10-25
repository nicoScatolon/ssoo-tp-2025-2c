#ifndef PLANIFICADORES_H
#define PLANIFICADORES_H
#include "globals.h"
#include "utils/globales.h"
#include "utils/logs.h"
#include <pthread.h>
#include "utils/paquete.h"
#include "qControl.h"
#include "workers.h"


void* planificadorFIFO();
void* planificadorPrioridad();
void* evaluarDesalojo();     
void* aging(void*arg);            
query*  obtenerQuery(void);                 
void    cambioEstado(t_list_mutex* lista, query* elemento);
query* sacarQueryDeMenorPrioridad();
query* sacarDePorId(t_list_mutex* l,int queryID);
bool estaEnListaPorId(t_list_mutex * lista, int id);
query* obtenerQueryMayorPrioridad();
query* obtenerQueryMenorPrioridad();
query* obtenerQueryPorID(int idQuery);
#endif