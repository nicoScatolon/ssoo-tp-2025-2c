#ifndef INICIALIZAR_H
#define INICIALIZAR_H
#include "globals.h"
#include "utils/globales.h"
#include "estructuras.h"
#include "bitmap.h"
#include "estructuras.h"
#include "hash.h"
void inicializarArchivo(const char *rutaBase, const char *nombre, const char *extension, char* modoApertura);
char* inicializarDirectorio(char* pathBase, char* nombreDirectorio);
void inicializarBloquesFisicos(char* pathPhysicalBlocks);
void inicializarMutex(void);
void inicializarBloqueCero(char* pathPhysicalBlocks);
char* crearHash(const char* contenido);

void levantarFileSystem(void);
void crearFile(char* nombreFile, char* nombreTag);
void crearTag(char* pathFile, char* nombreTag);
void crearMetaData(char* pathTag);
void inicializarMetaData(char* pathTag);
void cambiarEstadoMetaData(char* pathTag,char* estado);
void agregarBloqueMetaData(char* pathTag,int nuevoBloque);
extern int cantidadWorkers;

#endif