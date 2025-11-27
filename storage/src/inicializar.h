#ifndef INICIALIZAR_H
#define INICIALIZAR_H
#include "globals.h"
#include "utils/globales.h"
#include "estructuras.h"
#include "bitmap.h"
#include "estructuras.h"
#include "hash.h"
#include "metadata.h"
void inicializarArchivo(const char *rutaBase, const char *nombre, const char *extension, char* modoApertura);
char* inicializarDirectorio(char* pathBase, char* nombreDirectorio);
void inicializarBloquesFisicos(char* pathPhysicalBlocks);
void inicializarMutex(void);
void inicializarBloqueCero(char* pathPhysicalBlocks);
char* crearHash(const char* contenido);
void inicializarMetaData(char* pathTag);
void levantarFileSystem(void);
void eliminarFileSystemAnterior(void);
void eliminarDirectorioRecursivo(const char* path);
bool crearFile(char* nombreFile, char* nombreTag);
bool crearTag(char* pathFile, char* nombreTag);
void crearMetaData(char* pathTag);
void agregarBloqueMetaData(char* pathTag, int bloqueLogico,int nuevoBloqueFisico);
void cambiarEstadoMetaData(char* pathTag,char* estado);
void agregarBloqueMetaData(char* pathTag,int bloqueLogico,int nuevoBloqueFisico);
void agregarBloquesLogicos(char* pathTag, int tamanioArchivo);
void inicializarConexiones();
extern int cantidadWorkers;
extern t_list * listadoWorker;
#endif