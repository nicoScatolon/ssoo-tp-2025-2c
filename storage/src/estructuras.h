#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H

#include "configuracion.h"
#include "utils/config.h"
#include "utils/globales.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <commons/bitarray.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <commons/string.h>
	#include <stdbool.h>
	#include <stdarg.h>



typedef struct{
    char* hash;
    char* contenido;
    uint32_t numero;
}t_hash_block;

extern configStorage *configS;
extern configSuperBlock *configSB;
extern t_log* logger;
extern configStorage* configS;
extern configSuperBlock* configSB;
extern int numeroBloque;
extern pthread_mutex_t mutex_hash_block;
extern pthread_mutex_t mutex_numero_bloque;

extern int cantidadWorkers;
extern pthread_mutex_t mutex_cantidad_workers;

extern  t_bitarray* bitmap;  // bitarray de commons
extern  char* bitmap_buffer;  // área mapeada por mmap que contiene físicamente los bytes del bitmap.
extern size_t bitmap_size_bytes; // tamaño en bytes del bitmap

extern size_t bitmap_num_bits;     // cantidad total de bits (equal a cantidad de bloques)
extern int bitmap_fd;            // fd del archivo bitmap

extern pthread_mutex_t mutex_bitmap; 
extern pthread_mutex_t mutex_bitmap_file;


// Mutex por File:Tag
extern t_dictionary* mutex_file_tags;
extern pthread_mutex_t mutex_diccionario_file_tags;

pthread_mutex_t* obtenerMutexFileTag(char* file, char* tag);
void liberarMutexFileTag(char* file, char* tag);
void inicializarMutexFileTag(void);
void destruirMutexFileTag(void);


typedef struct {
    int socket;
    int workerId;
}t_worker;

extern t_list* listadoWorker;
extern pthread_mutex_t mutexWorkers;

extern t_log* logger;

// Variables para el manejo del bitmap


void liberarBloqueHash(t_hash_block * bloque);
t_hash_block* ocuparBloqueHash(const char *contenido, size_t contenido_len);
void incrementarNumeroBloque(void);

// Funciones para el manejo del bitmap
int bitmapSincronizar(void);
//void bitmapLiberarBloque(unsigned int index);
ssize_t bitmapReservarLibre(void);
int ocuparBloqueBit(int indice_bloque);
int liberarBloque(int indice_bloque, int queryID);
bool bitmapBloqueOcupado(unsigned int index);
void destruirBitmap(void);

#endif