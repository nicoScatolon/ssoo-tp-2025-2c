#ifndef CONEXIONES_H
#define CONEXIONES_H
#include "globales.h"


void agregarQueryControl(char* path,int socketCliente, int prioridad);
void agregarWorker(int socketCliente);
uint32_t generarIdQueryControl();
#endif

