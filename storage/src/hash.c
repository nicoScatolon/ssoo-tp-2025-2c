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