#ifndef GLOBALS_H
#define GLOBALS_H
#include "main.h"
#include "inicializar.h"
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
#endif