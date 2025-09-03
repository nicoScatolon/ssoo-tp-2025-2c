#ifndef CONEXIONES_H
#define CONEXIONES_H
#include "globales.h"


void agregarQueryControl(char* path,int socketCliente, int prioridad);
void agregarWorker(int socketCliente);
void agregarQuery(char* path,int prioridad);
uint32_t generarIdQueryControl();
uint32_t generarIdWorker();
uint32_t generarIdQuery();
#endif

