#ifndef OPERACIONES_H
#define OPERACIONES_H
#include "globals.h"
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

char* lecturaBloque(char* file, char* tag, int numeroBloque);
void tagFile(char* fileOrigen, char* tagOrigen, char* fileDestino, char* tagDestino, int queryId);
bool copiarDirectorioRecursivo(const char* origen, const char* destino);
bool copiarArchivo(const char* origen, const char* destino);

#endif 