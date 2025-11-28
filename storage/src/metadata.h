#ifndef METADATA_H
#define METADATA_H
#include "configuracion.h"
#include "inicializar.h"
void crearMetaData(char* pathTag,int tamanioInicial);
void inicializarMetaData(char* pathTag,int tamanioInicial);
void cambiarEstadoMetaData(char* pathTag,char* estado);
void agregarBloqueMetaData(char* pathTag, int bloqueLogico,int nuevoBloqueFisico);
#endif
