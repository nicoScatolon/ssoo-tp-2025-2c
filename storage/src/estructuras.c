#include "estructuras.h"

void vaciarMemoria(void){

}

void liberarBloqueHash(t_hash_block *bloque){
    if (!bloque) return;
    if (bloque->hash) {
        free(bloque->hash);
        bloque->hash = NULL;
    }
    free(bloque);
}

bool existeHash(char *hash){
    if (!hash) return false;

    char archivoPath[512];
    snprintf(archivoPath, sizeof(archivoPath), "%s/blocks_hash_index.config", configS->puntoMontaje);

    pthread_mutex_lock(&mutex_hash_block);

    t_config* config = config_create(archivoPath);
    if (config == NULL){
        log_error(logger, "No se pudo abrir blocks_hash_index.config (%s)", archivoPath);
        pthread_mutex_unlock(&mutex_hash_block);
        return false;
    }

    bool existe = config_has_property(config, hash);

    if (existe) log_debug(logger, "El hash ya existe: %s", hash);
    else       log_debug(logger, "El hash no existe: %s", hash);

    config_destroy(config);
    pthread_mutex_unlock(&mutex_hash_block);
    return existe;
}

void escribirBloqueHash(t_hash_block *bloque) {
    if (!bloque) return;
    
    char archivoPath[512];
    snprintf(archivoPath, sizeof(archivoPath), "%s/blocks_hash_index.config", configS->puntoMontaje);
    
    pthread_mutex_lock(&mutex_hash_block);

    FILE *archivo = fopen(archivoPath, "a+");
    if (archivo == NULL) {
        log_error(logger, "No se pudo abrir el archivo blocks_hash_index.config");
        pthread_mutex_unlock(&mutex_hash_block);
        liberarBloqueHash(bloque);
        return;
    }

    fprintf(archivo, "%s=%04%%d\n", bloque->hash, bloque->numero);
    log_debug(logger,"Escribiendo el bloque <%d> con hash <%s> ",bloque->numero,bloque->hash);
    liberarBloqueHash(bloque);
    fclose(archivo);
    pthread_mutex_unlock(&mutex_hash_block);
}

// crear/ocupar un bloque a partir de contenido: ahora recibe el contenido y su longitud.
// Esta versión ALLOCA y rellena t_hash_block y devuelve el puntero (propiedad al caller)
t_hash_block* ocuparBloque(const char *contenido, size_t contenido_len) {
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
    bloque->hash = crypto_md5(contenido, contenido_len);
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
int bitmapSincronizar(void) {
    if (!bitmap_buffer) return -1;
    if (msync(bitmap_buffer, bitmap_size_bytes, MS_SYNC) == -1) {
        log_error(logger, "Error sincronizando bitmap a disco: %s", strerror(errno));
        return -1;
    }
    return 0;
}

// liberar un bloque 
void bitmapLiberarBloque(unsigned int index) {
    if (!bitmap) return;
    if ((size_t)index >= bitmap_num_bits) return;
    bitarray_clean_bit(bitmap, (off_t)index);
}

bool bitmapBloqueOcupado(unsigned int index) {
    if (!bitmap) return false;
    if ((size_t)index >= bitmap_num_bits) return false;
    return bitarray_test_bit(bitmap, (off_t)index);
}

// reservar el primer bloque libre, retorna su indice o -1 si no hay libres
ssize_t bitmapReservarLibre(void) {
    if (!bitmap) return -1;
    for (off_t i = 0; i < (off_t)bitmap_num_bits; ++i) {
        if (!bitarray_test_bit(bitmap, i)) {
            bitarray_set_bit(bitmap, i);
            return (ssize_t)i;
        }
    }
    return -1;
}

// destruirBitmap: sincroniza y libera recursos del bitmap (llamar al terminar)
void destruirBitmap(void) {
    if (bitmapSincronizar() == -1) {
        log_error(logger, "Error sincronizando bitmap al destruir");
    }

    if (bitmap) {
        bitarray_destroy(bitmap); // destruye la estructura (no el buffer mapeado)
        bitmap = NULL;
    }
    if (bitmap_buffer) {
        munmap(bitmap_buffer, bitmap_size_bytes);
        bitmap_buffer = NULL;
    }
    if (bitmap_fd >= 0) {
        close(bitmap_fd);
        bitmap_fd = -1;
    }
    bitmap_size_bytes = 0;
    bitmap_num_bits = 0;
}