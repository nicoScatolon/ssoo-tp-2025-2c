#ifndef HASH_H
#define HASH_H
#include "configuracion.h"
#include "estructuras.h"
#include "inicializar.h"

void inicializarBlocksHashIndex(char* path);
void liberarBloqueHash(t_hash_block *bloque);
bool existeHash(char *hash);
void escribirHash(char* hash,int numeroBFisico);
char* calcularHashArchivo(char* path);
int obtenerBloquePorHash(char* hash);
void eliminarDelHashIndex(char* hash, int queryID);
#endif
