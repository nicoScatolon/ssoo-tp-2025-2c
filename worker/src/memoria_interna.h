#ifndef MEMORIA_INTERNA_H
#define MEMORIA_INTERNA_H

#include "utils/config.h"
#include "globals.h"

extern configWorker* configW;

char* inicializarMemoriaInterna(void);
void finalizarMemoriaInterna();
int ReemplazoLRU(); // Devuelve la página reemplazada
int ReemplazoCLOCKM(); // Devuelve la página reemplazada

#endif