#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H

#include "configuracion.h"
#include "utils/config.h"
#include "utils/globales.h"
#include <sys/stat.h>
#include <sys/types.h>
#include "globals.h"
#include <commons/bitarray.h>
#include <sys/mman.h>


typedef struct{
    char* hash;
    uint32_t numero;
    char* contenido;
}t_hash_block;

extern configStorage *configS;
extern configSuperBlock *configSB;

extern t_log* logger;

// Variables para el manejo del bitmap
static t_bitarray* bitmap = NULL; // bitarray de commons
static char* bitmap_buffer = NULL; // buffer en memoria
static size_t bitmap_size_bytes = 0; // tamaño en bytes del bitmap


void vaciarMemoria(void);
void liberarBloqueHash(t_hash_block * bloque);
bool existeHash(char * hash);
void escribirBloqueHash(t_hash_block *bloque);
void ocuparBloque(void);
void incrementarNumeroBloque(void);

// Funciones para el manejo del bitmap
void destruirBitmap(void);
int bitmap_setear(uint32_t index, bool ocupado, const char* path) ; // 0 ok, -1 error
int bitmap_test(uint32_t index, bool *out);     // 0 ok, -1 error

#endif