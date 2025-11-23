#ifndef OPERACIONES_H
#define OPERACIONES_H
#include "globals.h"
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

char* lecturaBloque(char* file, char* tag, int numeroBloque);
bool escribirBloque(char* file, char* tag, int numeroBloqueLogico, char* contenido, int queryID);
void actualizarHashBloque(int numeroBloqueFisico, char* contenido, int queryID);
bool truncarArchivo(char* file, char* tag, int nuevoTamanio, int queryID);
int contarReferenciasBloque(char* pathBloqueLogico, int queryID);
t_config* obtenerMetadata(char* file, char* tag, int queryID);
bool aumentarTamanioArchivo(char* file, char* tag, int bloquesActuales, int bloquesNuevos, int nuevoTamanio, t_config* metadata, int queryID);
bool reducirTamanioArchivo(char* file, char* tag, int bloquesActuales, int bloquesNuevos, int nuevoTamanio, t_config* metadata, int queryID);
bool hayEspacioDisponible(int bloquesNecesarios, int queryID);
void actualizarMetadataTruncate(t_config* metadata, int nuevoTamanio, int cantidadBloques);
bool tagFile(char* fileOrigen, char* tagOrigen, char* fileDestino, char* tagDestino, int queryId);
bool copiarDirectorioRecursivo(const char* origen, const char* destino);
bool copiarArchivo(const char* origen, const char* destino);
void aplicarRetardoBloque();
void aplicarRetardoOperacion();
bool eliminarTag(char* file, char* tag, int queryID);

void hacerCommited(char* file, char* tag, int queryID);
bool esCommited(char* file, char* tag,char*pathMetaData);
void recorrerBloquesLogicos(char* file, char* tag, int queryID);
int obtenerBloqueFisico(char* file, char* tag, int numeroBloqueLogico);
void procesarBloqueLogico(char* pathBloqueLogico, char* file, char* tag, int numeroBloqueLogico, int queryID);
void reapuntarBloqueLogico(char* file, char *tag, int numeroBloqueLogico,int previoBloqueFisico ,int nuevoBloqueFisico, int queryID);
int obtenerBloqueActual(char* file, char* tag, int numeroBloqueLogico);
void actualizarMetadataBloques(char* file, char* tag, int numeroBloqueLogico, int numeroNuevoBloqueFisico, int anteriorBloqueFisico, int queryID);
char* construirStringArray(char** array);
bool eliminarTag(char* file, char* tag, int queryID);
void eliminarBloqueLogico(char* pathBloqueLogico, char* file, char* tag, int numeroBloqueLogico, int queryID);
void eliminarBloquesLogicos(char* file, char* tag, int queryID);
#endif 