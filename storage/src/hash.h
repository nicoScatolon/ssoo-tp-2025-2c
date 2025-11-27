#ifndef HASH_H
#define HASH_H
#include "configuracion.h"
#include "globals.h"
#include "inicializar.h"
void inicializarBlocksHashIndex(char* path);
void liberarBloqueHash(t_hash_block *bloque);
bool existeHash(char *hash);
void escribirHash(char* hash,int numeroBFisico);
char* calcularHashArchivo(char* path);
int obtenerBloquePorHash(char* hash);
t_hash_block* ocuparBloqueHash(const char *contenido, size_t contenido_len);
void incrementarNumeroBloque(void);
#endif
