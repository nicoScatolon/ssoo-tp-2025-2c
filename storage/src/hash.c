#include "hash.h"

void inicializarBlocksHashIndex(char* path) {
    char archivoPath[512];
    snprintf(archivoPath, sizeof(archivoPath), "%s/blocks_hash_index.config", path);
    inicializarArchivo(path, "blocks_hash_index", "config", "w");
    log_debug(logger, "Archivo blocks_hash_index.config inicializado correctamente");
}


