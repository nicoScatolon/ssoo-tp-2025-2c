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
