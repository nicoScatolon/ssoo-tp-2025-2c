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


// -----------------------------------------------------------------------------
// Funciones para el manejo del bitmap
// -----------------------------------------------------------------------------

// destruirBitmap: sincroniza y libera recursos del bitmap (llamar al terminar)
void destruirBitmap(void) {
    if (bitmap) {
        bitarray_destroy(bitmap); // destruye la estructura
        bitmap = NULL;
    }

    if (bitmap_buffer) {
        free(bitmap_buffer);
        bitmap_buffer = NULL;
    }

    bitmap_size_bytes = 0;
    log_info(logger, "Bitmap destruido correctamente");
}


// bitmap_setear: setea o limpia un bit en el bitmap persistente
// retorna 0 en exito, -1 en error
int bitmap_setear(uint32_t index, bool ocupado, const char* path) {
    if (!bitmap || !bitmap_buffer) {
        log_error(logger, "bitmap_setear: bitmap no inicializado");
        return -1;
    }

    uint32_t cantBloques = configSB->FS_SIZE / configSB->BLOCK_SIZE;
    if (index >= cantBloques) {
        log_error(logger, "bitmap_setear: indice %u fuera de rango (max %u)", index, cantBloques - 1);
        return -1;
    }

    if (ocupado)
        bitarray_set_bit(bitmap, (off_t)index);
    else
        bitarray_clean_bit(bitmap, (off_t)index);

    // Guardar cambios en el archivo
    char bitmapPath[512];
    snprintf(bitmapPath, sizeof(bitmapPath), "%s/bitmap.bin", path);

    FILE* archivo = fopen(bitmapPath, "r+b");
    if (!archivo) {
        log_error(logger, "bitmap_setear: no se pudo abrir %s: %s", bitmapPath, strerror(errno));
        return -1;
    }

    size_t written = fwrite(bitmap->bitarray, 1, bitmap_size_bytes, archivo);
    fflush(archivo);
    fclose(archivo);

    if (written != bitmap_size_bytes) {
        log_error(logger, "bitmap_setear: no se pudieron escribir todos los bytes al archivo");
        return -1;
    }

    return 0;
}

// bitmap_test: consulta un bit
// out = true si ocupado, false si libre
int bitmap_test(uint32_t index, bool* out) {
    if (!bitmap || !bitmap_buffer) {
        log_error(logger, "bitmap_test: bitmap no inicializado");
        return -1;
    }

    uint32_t cantBloques = configSB->FS_SIZE / configSB->BLOCK_SIZE;
    if (index >= cantBloques) {
        log_error(logger, "bitmap_test: indice %u fuera de rango (max %u)", index, cantBloques - 1);
        return -1;
    }

    *out = bitarray_test_bit(bitmap, (off_t)index);
    return 0;
}