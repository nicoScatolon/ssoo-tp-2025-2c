#include "hash.h"

void inicializarBlocksHashIndex(char* path) {
    char archivoPath[512];
    snprintf(archivoPath, sizeof(archivoPath), "%s/blocks_hash_index.config", path);
    inicializarArchivo(path, "blocks_hash_index", "config", "w");
    log_debug(logger, "Archivo blocks_hash_index.config inicializado correctamente");
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

void escribirHash(char* hash,int numeroBFisico) {
    
    char archivoPath[512];
    snprintf(archivoPath, sizeof(archivoPath), "%s/blocks_hash_index.config", configS->puntoMontaje);
    
    pthread_mutex_lock(&mutex_hash_block);

    FILE *archivo = fopen(archivoPath, "a");
    if (archivo == NULL) {
        log_error(logger, "No se pudo abrir el archivo blocks_hash_index.config");
        pthread_mutex_unlock(&mutex_hash_block);
        exit(EXIT_FAILURE);
    }

    fprintf(archivo, "%s=block%04d\n", hash, numeroBFisico); 
    log_debug(logger,"Escribiendo el bloque <%d> con hash <%s> ",numeroBFisico,hash);
    fclose(archivo);
    pthread_mutex_unlock(&mutex_hash_block);
}

char* calcularHashArchivo(char* path) {
    FILE* archivo = fopen(path, "rb");
    if (archivo == NULL) {
        log_error(logger, "Error al abrir archivo para hash: %s", path);
        exit(EXIT_FAILURE);  
    }
    
    // Leer todo el contenido
    fseek(archivo, 0, SEEK_END);
    long tamanio = ftell(archivo);
    fseek(archivo, 0, SEEK_SET);
    
    char* contenido = malloc(tamanio + 1);  
    if (contenido == NULL) {
        fclose(archivo);
        return NULL;
    }
    
    fread(contenido, 1, tamanio, archivo);
    contenido[tamanio] = '\0';  
    fclose(archivo);
    
    char* hash = crypto_md5((void*)contenido, tamanio);
    free(contenido);
    return hash;
}

int obtenerBloquePorHash(char* hash) {    
    char* archivoPath = string_from_format("%s/blocks_hash_index.config",configS->puntoMontaje);
    
    pthread_mutex_lock(&mutex_hash_block);
    t_config* config = config_create(archivoPath);
    
    if (config == NULL) {
        log_error(logger, "No se pudo abrir blocks_hash_index.config");
        pthread_mutex_unlock(&mutex_hash_block);
        free(archivoPath);
        exit(EXIT_FAILURE);
    }
    
    int bloque = -1;
    if (config_has_property(config, hash)) {
        char* valor = config_get_string_value(config, hash);
        
        // Extraer el número después de "block"
        if (strncmp(valor, "block", 5) == 0) {
            bloque = atoi(valor + 5);
        }
    }
    
    config_destroy(config);
    pthread_mutex_unlock(&mutex_hash_block);
    free(archivoPath);
    
    return bloque;
}


void eliminarDelHashIndex(char* hash, int queryID) {
    char* pathHashIndex = string_from_format("%s/blocks_hash_index.config", configS->puntoMontaje);
    pthread_mutex_lock(&mutex_hash_block);
    t_config* hashIndex = config_create(pathHashIndex);
    if (hashIndex == NULL) {
        log_error(logger, "## <%d> - Error al abrir blocks_hash_index.config", queryID);
        free(pathHashIndex);
        return;
    }
    
    // Verificar si existe la clave
    if (config_has_property(hashIndex, hash)) {
        config_remove_key(hashIndex, hash);
        config_save(hashIndex);
        log_debug(logger, "## <%d> - Se eliminó entrada del hash_index: %s", queryID, hash);
    } else {
        log_debug(logger, "## <%d> - Hash %s no encontrado en hash_index", queryID, hash);
    }
    pthread_mutex_unlock(&mutex_hash_block);
    config_destroy(hashIndex);
    free(pathHashIndex);
}