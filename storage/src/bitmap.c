#include "bitmap.h"

int inicializarBitmap(const char* bitmap_path) {
    if (configSB == NULL) {
        log_error(logger, "configSB no inicializado");
        return -1;
    }

    size_t fs_size = configSB->FS_SIZE;
    size_t block_size = configSB->BLOCK_SIZE;

    if (block_size == 0) {
        log_error(logger, "BLOCK_SIZE inválido (0)");
        return -1;
    }

    // cantidad de bloques del FS
    size_t cant_bloques = fs_size / block_size;
    if (cant_bloques == 0) {
        log_error(logger, "FS demasiado pequeño (%zu bytes) o BLOCK_SIZE demasiado grande (%zu bytes)",
                  fs_size, block_size);
        return -1;
    }

    size_t size_bytes = cant_bloques / 8;

    pthread_mutex_lock(&mutex_bitmap_file);

    FILE* archivo = fopen(bitmap_path, "a+");
    if (!archivo) {
        fprintf(stderr, "No se pudo abrir/crear '%s': %s\n", bitmap_path, strerror(errno));
        pthread_mutex_unlock(&mutex_bitmap_file);
        return -1;
    }

    int fd = fileno(archivo);
    if (fd == -1) {
        fprintf(stderr, "Error al obtener file descriptor: %s\n", strerror(errno));
        fclose(archivo);
        pthread_mutex_unlock(&mutex_bitmap_file);
        return -1;
    }

    if (ftruncate(fd, (off_t)size_bytes) == -1) {
        fprintf(stderr, "Error ajustando tamaño del bitmap: %s\n", strerror(errno));
        fclose(archivo);
        pthread_mutex_unlock(&mutex_bitmap_file);
        return -1;
    }

    void* mapped = mmap(NULL, size_bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        fprintf(stderr, "Error al mapear '%s': %s\n", bitmap_path, strerror(errno));
        fclose(archivo);
        pthread_mutex_unlock(&mutex_bitmap_file);
        return -1;
    }

    bitmap_fd = fd;
    bitmap_buffer = (char*)mapped;
    bitmap_size_bytes = size_bytes;
    bitmap_num_bits = cant_bloques;

    bitmap = bitarray_create_with_mode(bitmap_buffer, bitmap_size_bytes, LSB_FIRST);
    if (!bitmap) {
        fprintf(stderr, "Error creando bitarray\n");
        munmap(bitmap_buffer, bitmap_size_bytes);
        fclose(archivo);
        pthread_mutex_unlock(&mutex_bitmap_file);
        return -1;
    }

    fclose(archivo);
    pthread_mutex_unlock(&mutex_bitmap_file);

    log_info(logger, "Bitmap inicializado correctamente: %zu bloques (%zu bytes en archivo '%s')",
             cant_bloques, size_bytes, bitmap_path);

    return 0;
}
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

int ocuparBloqueBit(int indice_bloque) {
    pthread_mutex_lock(&mutex_bitmap_file);

    if (bitmap == NULL) {
        log_error(logger, "Bitmap no inicializado");
        pthread_mutex_unlock(&mutex_bitmap_file);
        return -1;
    }

    if (indice_bloque < 0 || indice_bloque >= bitmap_num_bits) {
        log_error(logger, "Índice de bloque inválido: %d", indice_bloque);
        pthread_mutex_unlock(&mutex_bitmap_file);
        return -1;
    }

    bitarray_set_bit(bitmap, indice_bloque);

    // Sincronizamos cambios al archivo
    if (msync(bitmap_buffer, bitmap_size_bytes, MS_SYNC) == -1) {
        log_error(logger, "Error al sincronizar bitmap: %s", strerror(errno));
        pthread_mutex_unlock(&mutex_bitmap_file);
        return -1;
    }

    log_debug(logger, "Bloque %d marcado como OCUPADO", indice_bloque);
    pthread_mutex_unlock(&mutex_bitmap_file);
    return 0;
}

int liberarBloque(int indice_bloque, int queryID) {
    pthread_mutex_lock(&mutex_bitmap_file);

    if (bitmap == NULL) {
        log_error(logger, "Bitmap no inicializado");
        pthread_mutex_unlock(&mutex_bitmap_file);
        return -1;
    }

    if (indice_bloque < 0 || indice_bloque >= bitmap_num_bits) {
        log_error(logger, "Índice de bloque inválido: %d", indice_bloque);
        pthread_mutex_unlock(&mutex_bitmap_file);
        return -1;
    }

    bitarray_clean_bit(bitmap, indice_bloque);

    // Sincronizamos cambios al archivo
    if (msync(bitmap_buffer, bitmap_size_bytes, MS_SYNC) == -1) {
        log_error(logger, "Error al sincronizar bitmap: %s", strerror(errno));
        pthread_mutex_unlock(&mutex_bitmap_file);
        return -1;
    }

    log_info(logger, "##<%d> - Bloque Físico Liberado - Número de Bloque: <%d>", queryID, indice_bloque);
    pthread_mutex_unlock(&mutex_bitmap_file);
    return 0;
}
ssize_t bitmapReservarLibre(void) {
    pthread_mutex_lock(&mutex_bitmap_file);

    if (!bitmap) {
        log_error(logger, "Bitmap no inicializado");
        pthread_mutex_unlock(&mutex_bitmap_file);
        return -1;
    }

    for (off_t i = 0; i < (off_t)bitmap_num_bits; ++i) {
        if (!bitarray_test_bit(bitmap, i)) {
            bitarray_set_bit(bitmap, i);

            if (msync(bitmap_buffer, bitmap_size_bytes, MS_SYNC) == -1) {
                log_error(logger, "Error sincronizando bitmap tras reservar bloque: %s", strerror(errno));
            } else {
                log_debug(logger, "Bloque %ld reservado correctamente", i);
            }

            pthread_mutex_unlock(&mutex_bitmap_file);
            return (ssize_t)i;
        }
    }

    log_warning(logger, "No hay bloques libres en el bitmap");
    pthread_mutex_unlock(&mutex_bitmap_file);
    return -1;
}
bool bitmapBloqueOcupado(unsigned int index) {
    pthread_mutex_lock(&mutex_bitmap_file);

    bool ocupado = false;

    if (!bitmap) {
        log_error(logger, "Bitmap no inicializado");
    } else if ((size_t)index >= bitmap_num_bits) {
        log_error(logger, "Índice de bloque inválido: %u", index);
    } else {
        ocupado = bitarray_test_bit(bitmap, (off_t)index);
    }

    pthread_mutex_unlock(&mutex_bitmap_file);
    return ocupado;
}
int bitmapSincronizar(void) {
    if (!bitmap_buffer) return -1;
    if (msync(bitmap_buffer, bitmap_size_bytes, MS_SYNC) == -1) {
        log_error(logger, "Error sincronizando bitmap a disco: %s", strerror(errno));
        return -1;
    }
    return 0;
}