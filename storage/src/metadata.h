#ifndef METADATA_H
#define METADATA_H
#include "configuracion.h"
#include "globals.h"
#include "inicializar.h"
void crearMetaData(char* pathTag);
void inicializarMetaData(char* pathTag);
void cambiarEstadoMetaData(char* pathTag,char* estado);
void agregarBloqueMetaData(char* pathTag, int bloqueLogico,int nuevoBloqueFisico);
#endif
