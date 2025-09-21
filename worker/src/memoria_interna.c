#include <memoria_interna.h>


void inicializarMemoriaInterna(void) {
    pthread_mutex_lock(&memoria_mutex);

    if (memoria != NULL) { // ya inicializada
        pthread_mutex_unlock(&memoria_mutex);
        return;
    }

    int tam_memoria = configW->tamMemoria;
    int tam_pagina  = configW->BLOCK_SIZE;
    if (tam_pagina <= 0) {
        log_error(logger, "BLOCK_SIZE invalido: %d", tam_pagina);
        pthread_mutex_unlock(&memoria_mutex);
        exit(EXIT_FAILURE);
    }

    cant_paginas = tam_memoria / tam_pagina;
    if (cant_paginas <= 0) {
        log_error(logger, "Tam memoria insuficiente para pagina de %d bytes", tam_pagina);
        pthread_mutex_unlock(&memoria_mutex);
        exit(EXIT_FAILURE);
    }

    memoria = calloc(1, tam_memoria);
    if (memoria == NULL) {
        log_error(logger, "Error al asignar memoria interna de tamaño %d", tam_memoria);
        pthread_mutex_unlock(&memoria_mutex);
        exit(EXIT_FAILURE);
    }

    int bytes_bitmap = (cant_paginas + 7) / 8; // redondeo hacia arriba
    void* bitmap_mem = calloc(1, bytes_bitmap);
    if (bitmap_mem == NULL) {
        log_error(logger, "Error calloc bitmap (%d bytes)", bytes_bitmap);
        free(memoria);
        memoria = NULL;
        pthread_mutex_unlock(&memoria_mutex);
        exit(EXIT_FAILURE);
    }
    
    bitmap = bitarray_create_with_mode(bitmap_mem, bytes_bitmap, LSB_FIRST); //revisar con valgrind si hay memory leak
    if (bitmap == NULL) {
        log_error(logger, "Error creando bitarray");
        free(bitmap_mem);
        free(memoria);
        memoria = NULL;
        pthread_mutex_unlock(&memoria_mutex);
        exit(EXIT_FAILURE);
    }

    log_debug(logger, "Memoria interna inicializada: %d bytes, %d paginas, bitmap de %d bytes",
              tam_memoria, cant_paginas, bytes_bitmap);

    pthread_mutex_unlock(&memoria_mutex);
}

void finalizarMemoriaInterna(void) {
    pthread_mutex_lock(&memoria_mutex);

    if (bitmap) {
        bitarray_destroy(bitmap);
        bitmap = NULL;
    }

    if (memoria) {
        free(memoria);
        memoria = NULL;
    }

    cant_paginas = 0;

    pthread_mutex_unlock(&memoria_mutex);
}

int obtenerPaginaLibre(void){
    pthread_mutex_lock(&memoria_mutex);

    for (int i = 0; i < cant_paginas; i++) {
        if (!bitarray_test_bit(bitmap, i)) { // Página libre
            bitarray_set_bit(bitmap, i); // Marcar como usada
            pthread_mutex_unlock(&memoria_mutex);
            return i; // Retornar número de página libre
        }
    }

    pthread_mutex_unlock(&memoria_mutex);
    return -1; // No hay páginas libres
}

void liberarPagina(int nro_pagina){
    pthread_mutex_lock(&memoria_mutex);

    if (nro_pagina < 0 || nro_pagina >= cant_paginas) {
        log_error(logger, "Número de página inválido: %d", nro_pagina);
        pthread_mutex_unlock(&memoria_mutex);
        return;
    }

    bitarray_clean_bit(bitmap, nro_pagina);

    pthread_mutex_unlock(&memoria_mutex);
}

void reservarPagina(int nro_pagina){
    pthread_mutex_lock(&memoria_mutex);

    if (nro_pagina < 0 || nro_pagina >= cant_paginas) {
        log_error(logger, "Número de página inválido: %d", nro_pagina);
        pthread_mutex_unlock(&memoria_mutex);
        return;
    }

    if (bitarray_test_bit(bitmap, nro_pagina))
    {
        log_warning(logger, "Página %d ya está reservada", nro_pagina);
        pthread_mutex_unlock(&memoria_mutex);
        return;
    }

    bitarray_set_bit(bitmap, nro_pagina);

    pthread_mutex_unlock(&memoria_mutex);
}



int ReemplazoLRU() {
    // Implementación del algoritmo de reemplazo LRU
    return -1; // Devuelve la página reemplazada (ejemplo)
}

int ReemplazoCLOCKM() {
    // Implementación del algoritmo de reemplazo CLOCK-M
    return -1; // Devuelve la página reemplazada (ejemplo)
}
