#include "globals.h"
#include <commons/bitarray.h>


// Definiciones de las variables globales de globals.h


// Mutexes del bitmap
// Definiciones de las variables globales
pthread_mutex_t mutex_hash_block = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_numero_bloque = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cantidad_workers = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_bitmap = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_bitmap_file = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexWorkers = PTHREAD_MUTEX_INITIALIZER;
int cantidadWorkers = 0;
int numeroBloque = 0;

// Definiciones de las variables del bitmap
t_bitarray* bitmap = NULL;
char* bitmap_buffer = NULL;
size_t bitmap_size_bytes = 0;
size_t bitmap_num_bits = 0;
int bitmap_fd = -1;

t_list* listadoWorker= NULL;