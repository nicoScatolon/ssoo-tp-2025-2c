#include "estructuras.h"

void vaciarMemoria(void){

}

void liberarBloqueHash(t_hash_block * bloque){
    free(bloque->hash);
    free(bloque);
}

bool existeHash(char * hash){
    char archivoPath[512];
    snprintf(archivoPath, sizeof(archivoPath), "%s/blocks_hash_index.config", configS->puntoMontaje);
    pthread_mutex_lock(&mutex_hash_block);
    t_config* config = config_create(archivoPath);
    if (config == NULL){
        pthread_mutex_unlock(&mutex_hash_block);
        log_error(logger, "No se pudo abrir el archivo blocks_hash_index.config");
        exit (EXIT_FAILURE);
    }
    if(config_has_property(config,hash)){
        log_debug(logger,"El hash ya existe");
        pthread_mutex_unlock(&mutex_hash_block);
        config_destroy(config);
        return true;
    }
    else{
        log_debug(logger,"El hash no existe");
        pthread_mutex_unlock(&mutex_hash_block);
        config_destroy(config);
        return false;
    }
}

void escribirBloqueHash(t_hash_block *bloque) {
    char archivoPath[512];
    snprintf(archivoPath, sizeof(archivoPath), "%s/blocks_hash_index.config", configS->puntoMontaje);
    pthread_mutex_lock(&mutex_hash_block);

    FILE *archivo = fopen(archivoPath, "a+");
    if (archivo == NULL) {
        log_error(logger, "No se pudo abrir el archivo blocks_hash_index.config");
        pthread_mutex_unlock(&mutex_hash_block);
        exit(EXIT_FAILURE);
    }
    fprintf(archivo, "%s=%04%%d\n", bloque->hash, bloque->numero);
    log_debug(logger,"Escribiendo el bloque <%d> con hash <%s> ",bloque->numero,bloque->hash);
    liberarBloqueHash(bloque);
    fclose(archivo);
    pthread_mutex_unlock(&mutex_hash_block);
}

void ocuparBloque(void){
    t_hash_block* bloque = malloc(sizeof(t_hash_block));
    bloque->numero= numeroBloque;
    incrementarNumeroBloque();
    bloque->hash= crypto_md5(bloque->contenido,strlen(bloque->contenido) + 1);
}

void incrementarNumeroBloque(void){
    pthread_mutex_lock(&mutex_numero_bloque);
    numeroBloque++;
    pthread_mutex_unlock(&mutex_numero_bloque);
}