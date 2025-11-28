#include "estructuras.h"

int numeroBloque = 0;
pthread_mutex_t mutex_hash_block = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_numero_bloque = PTHREAD_MUTEX_INITIALIZER;
int cantidadWorkers = 0;
pthread_mutex_t mutex_cantidad_workers = PTHREAD_MUTEX_INITIALIZER;

// Definiciones de las variables del bitmap
t_bitarray* bitmap = NULL;
char* bitmap_buffer = NULL;
size_t bitmap_size_bytes = 0;
size_t bitmap_num_bits = 0;
int bitmap_fd = -1;
t_list* listadoWorker;
pthread_mutex_t mutexWorkers = PTHREAD_MUTEX_INITIALIZER;
// Mutexes del bitmap
pthread_mutex_t mutex_bitmap = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_bitmap_file = PTHREAD_MUTEX_INITIALIZER;

// Mutex para el metadata
pthread_mutex_t mutex_metadata = PTHREAD_MUTEX_INITIALIZER;


// crear/ocupar un bloque a partir de contenido: ahora recibe el contenido y su longitud.
// Esta versión ALLOCA y rellena t_hash_block y devuelve el puntero (propiedad al caller)
t_hash_block* ocuparBloqueHash(const char *contenido, size_t contenido_len) {
    if (!contenido) return NULL;

    t_hash_block* bloque = malloc(sizeof(t_hash_block));
    if (!bloque) {
        log_error(logger, "malloc fallo en ocuparBloque");
        return NULL;
    }

    // asignar numero actual y luego incrementar de forma thread-safe
    pthread_mutex_lock(&mutex_numero_bloque);
    bloque->numero = numeroBloque;
    numeroBloque++;
    pthread_mutex_unlock(&mutex_numero_bloque);

    // duplico contenido si querés conservarlo en la estructura (opcional)
    // bloque->contenido = strndup(contenido, contenido_len);
    // Si preferís no duplicar, asigná NULL o apunta fuera y documentalo.
    bloque->contenido = NULL; // <-- por ahora no alojamos el contenido dentro del struct

    // crypto_md5 espera un buffer y su longitud (según commons), y devuelve char* malloc'd.
    // Asegurate de pasar la longitud correcta; NO sumar +1 para el terminador si querés
    // que el digest sea del contenido crudo (si querés incluir '\0' tocá eso explícitamente).
    bloque->hash = crypto_md5((void*)contenido, contenido_len);
    if (!bloque->hash) {
        log_error(logger, "crypto_md5 falló al calcular hash");
        free(bloque);
        return NULL;
    }

    log_debug(logger, "Ocupado bloque %d con hash %s", bloque->numero, bloque->hash);
    return bloque;
}

void incrementarNumeroBloque(void){
    pthread_mutex_lock(&mutex_numero_bloque);
    numeroBloque++;
    pthread_mutex_unlock(&mutex_numero_bloque);
}


// -----------------------------------------------------------------------------
// Funciones para el manejo del bitmap
// -----------------------------------------------------------------------------


// liberar un bloque 
// void bitmapLiberarBloque(unsigned int index) {
//     if (!bitmap) return;
//     if ((size_t)index >= bitmap_num_bits) return;
//     bitarray_clean_bit(bitmap, (off_t)index);
// }

// bool bitmapBloqueOcupado(unsigned int index) {
//     if (!bitmap) return false;
//     if ((size_t)index >= bitmap_num_bits) return false;
//     return bitarray_test_bit(bitmap, (off_t)index);
// }

// // reservar el primer bloque libre, retorna su indice o -1 si no hay libres
// ssize_t bitmapReservarLibre(void) {
//     if (!bitmap) return -1;
//     for (off_t i = 0; i < (off_t)bitmap_num_bits; ++i) {
//         if (!bitarray_test_bit(bitmap, i)) {
//             bitarray_set_bit(bitmap, i);
//             return (ssize_t)i;
//         }
//     }
//     return -1;
// }

// destruirBitmap: sincroniza y libera recursos del bitmap (llamar al terminar)
