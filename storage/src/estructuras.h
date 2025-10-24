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
#include <fcntl.h>

typedef struct{
    char* hash;
    char* contenido;
    uint32_t numero;
}t_hash_block;

extern configStorage *configS;
extern configSuperBlock *configSB;

extern t_log* logger;

// Variables para el manejo del bitmap
static t_bitarray* bitmap = NULL; // bitarray de commons
static char* bitmap_buffer = NULL; // área mapeada por mmap que contiene físicamente los bytes del bitmap.
static size_t bitmap_size_bytes = 0; // tamaño en bytes del bitmap

static size_t bitmap_num_bits = 0;     // cantidad total de bits (equal a cantidad de bloques)
static int bitmap_fd = -1;             // fd del archivo bitmap

pthread_mutex_t mutex_bitmap = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_bitmap_file = PTHREAD_MUTEX_INITIALIZER;

void vaciarMemoria(void);
void liberarBloqueHash(t_hash_block * bloque);
bool existeHash(char * hash);
void escribirHash(char* hash,int numeroBFisico);
t_hash_block* ocuparBloqueHash(const char *contenido, size_t contenido_len);
void incrementarNumeroBloque(void);

// Funciones para el manejo del bitmap
int bitmapSincronizar(void);
//void bitmapLiberarBloque(unsigned int index);
ssize_t bitmapReservarLibre(void);
int ocuparBloqueBit(int indice_bloque);
int liberarBloque(int indice_bloque);
bool bitmapBloqueOcupado(unsigned int index);
void destruirBitmap(void);

#endif