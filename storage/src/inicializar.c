#include "inicializar.h"

//Inicializar archivo generico
void inicializarArchivo(const char *rutaBase, const char *nombre, const char *extension,char* modoApertura) {
    char archivoPath[512];
    snprintf(archivoPath, sizeof(archivoPath), "%s/%s.%s", rutaBase, nombre, extension);

    FILE* archivo = fopen(archivoPath, modoApertura);
    if (archivo == NULL) {
        fprintf(stderr, "No se pudo crear/abrir %s: %s\n", archivoPath, strerror(errno));
        exit(EXIT_FAILURE);
    }
    fclose(archivo);
    if (modoApertura[0] == 'w' || modoApertura[0] == 'a') {
        if (chmod(archivoPath, 0777) != 0) {
            log_error(logger, "No se pudieron asignar permisos a %s: \n", archivoPath);
        }
    }
}

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

void inicializarBlocksHashIndex(char* path) {
    char archivoPath[512];
    snprintf(archivoPath, sizeof(archivoPath), "%s/blocks_hash_index.config", path);

    // Si ya existe, no hacer nada
    if (access(archivoPath, F_OK) == 0) {
        log_debug(logger, "Archivo blocks_hash_index.config ya existe, no se sobrescribe");
        return;
    }

    // Crear archivo vacío
    inicializarArchivo(path, "blocks_hash_index", "config", "w");
    log_debug(logger, "Archivo blocks_hash_index.config inicializado correctamente");
}

char* inicializarDirectorio(char* pathBase, char* nombreDirectorio){ 
    char dirPath[512];
    snprintf(dirPath, sizeof(dirPath), "%s/%s", pathBase, nombreDirectorio);

    // mkdir devuelve 0 si creó el directorio, -1 si hubo error
    if (mkdir(dirPath, 0777) == -1) {
        if (errno != EEXIST) { // si el directorio ya existe, no es error
            log_error(logger, "No se pudo crear el directorio %s: %s", nombreDirectorio, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    return strdup(dirPath);
}

void inicializarFileSystem(char* pathPhysicalBlocks) {
    if (configS->freshStart) {
        int cantBloques = configSB->FS_SIZE / configSB->BLOCK_SIZE;

        for (int i = 0; i < cantBloques; i++) {
            char nombreBloque[32];
            // Generar: block0000, block0001, ..., block9999
            snprintf(nombreBloque, sizeof(nombreBloque), "block%04d", i);

            inicializarArchivo(pathPhysicalBlocks, nombreBloque, "dat", "w+");
        }
    }
}

void inicializarMutex(void){
    pthread_mutex_init(&mutex_hash_block,NULL);
    pthread_mutex_init(&mutex_numero_bloque,NULL);
}

void inicializarEstructuras(void){
    char* path = configS->puntoMontaje;

    log_debug(logger,"Punto de montaje: %s",path);

    

    // Directorios
    char* pathPhysicalBlocks = inicializarDirectorio(path, "physical_blocks"); //terminado
    char* pathFiles = inicializarDirectorio(path, "files"); //hacer

    //inicializarSuperblock(); //se debe hacer en las configs

    // Archivos
    inicializarFileSystem(pathPhysicalBlocks);
    inicializarBitmap(path);
    inicializarBlocksHashIndex(path);

    log_debug(logger,"Estructuras inicializadas correctamente");

    free(path);
    free(pathPhysicalBlocks);
    free(pathFiles);
}
