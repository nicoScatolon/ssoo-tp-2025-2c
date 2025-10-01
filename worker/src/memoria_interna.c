#include <memoria_interna.h>


void inicializarMemoriaInterna(void) {
    pthread_mutex_lock(&memoria_mutex);

    if (memoria) { // ya inicializada
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

void inicializarDiccionarioDeTablas(void) {
    pthread_mutex_lock(&tabla_paginas_mutex);
    if (!tablasDePaginas){
        tablasDePaginas = dictionary_create();
        if (!tablasDePaginas) {
            log_error(logger, "Error al crear el diccionario de tablas de paginas");
            pthread_mutex_unlock(&tabla_paginas_mutex);
            exit(EXIT_FAILURE);
        }
    }
    else {
        log_warning(logger, "El diccionario de tablas de paginas ya fue inicializado");
    }
    pthread_mutex_unlock(&tabla_paginas_mutex);
}

char* pidToKey(int pid) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", pid);
    return strdup(buf);
}

void agregarProcesoADiccionarioDeTablas(int pid){
    pthread_mutex_lock(&tabla_paginas_mutex);

    char* key = pidToKey(pid);
    if (dictionary_has_key(tablasDePaginas, key)) {
        log_warning(logger, "La tabla de paginas para el proceso %d ya existe", pid);
        free(key);
        pthread_mutex_unlock(&tabla_paginas_mutex);
        return;
    }

    TablaPagina* tabla = malloc(sizeof(TablaPagina)); // me genera dudas
    if (tabla == NULL) {
        log_error(logger, "Error al asignar memoria para la tabla de paginas del proceso %d", pid);
        free(key);
        pthread_mutex_unlock(&tabla_paginas_mutex);
        return;
    }

    dictionary_put(tablasDePaginas, key, tabla);
    log_debug(logger, "Tabla de paginas creada para el proceso %d", pid);

    free(key);
    pthread_mutex_unlock(&tabla_paginas_mutex);
}

void pedirBloqueAStorage(){}//TODO

void agregarPaginaAProceso(int pid, int nro_pagina){ //Terminar - Responsabilidad del storage
    pthread_mutex_lock(&tabla_paginas_mutex);

    char* key = pidToKey(pid);
    TablaPagina* tabla = dictionary_get(tablasDePaginas, key);
    if (tabla == NULL) {
        log_warning(logger, "No existe la tabla de paginas para el proceso %d", pid);
        free(key);
        pthread_mutex_unlock(&tabla_paginas_mutex);
        
        agregarProcesoADiccionarioDeTablas(pid);
        agregarPaginaAProceso(pid, nro_pagina); 
        return;
    }

    // Aquí deberías agregar la lógica para añadir la página a la tabla
    // Por ejemplo, podrías reallocar el array de entradas y añadir una nueva entrada

    log_debug(logger, "Página %d agregada a la tabla del proceso %d", nro_pagina, pid);

    free(key);
    pthread_mutex_unlock(&tabla_paginas_mutex);
}


void eliminarMemoriaInterna(void) {
    pthread_mutex_lock(&memoria_mutex);

    if (bitmap) {
        bitarray_destroy(bitmap);
        bitmap = NULL;
    }

    if (memoria) {
        free(memoria);
        memoria = NULL;
    }

    pthread_mutex_lock(&tabla_paginas_mutex);
    if (tablasDePaginas) {
        dictionary_destroy_and_destroy_elements(tablasDePaginas, free);
        tablasDePaginas = NULL;
    }
    pthread_mutex_unlock(&tabla_paginas_mutex);

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

    int offset = nro_pagina * configW->BLOCK_SIZE;
    memset(&memoria[offset], 0, configW->BLOCK_SIZE);

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

    //se debería agregar a la tabla de paginas del proceso

    pthread_mutex_unlock(&memoria_mutex);
}


// Lectura/Escritura desde la "Memoria Interna"
char leerDesdeMemoriaByte(const char* nombreFile, const char* tag, int nroPagina, int offset){
    pthread_mutex_lock(&memoria_mutex);

    int tam_pagina = configW->BLOCK_SIZE;
    if (nroPagina < 0 || nroPagina >= cant_paginas || offset < 0 || offset >= tam_pagina) {
        log_error(logger, "Acceso inválido a memoria: página %d, offset %d", nroPagina, offset);
        pthread_mutex_unlock(&memoria_mutex);
        return -1; // Valor inválido
    }

    char valor = memoria[nroPagina * tam_pagina + offset];

    pthread_mutex_unlock(&memoria_mutex);
    return valor;
}

void escribirEnMemoriaByte(const char* nombreFile, const char* tag, int nroPagina, int offset, char valor){
    pthread_mutex_lock(&memoria_mutex);

    int tam_pagina = configW->BLOCK_SIZE;
    if (nroPagina < 0 || nroPagina >= cant_paginas || offset < 0 || offset >= tam_pagina) {
        log_error(logger, "Acceso inválido a memoria: página %d, offset %d", nroPagina, offset);
        pthread_mutex_unlock(&memoria_mutex);
        return;
    }

    memoria[nroPagina * tam_pagina + offset] = valor;

    pthread_mutex_unlock(&memoria_mutex);
}

void* leerDesdeMemoriaPaginaCompleta(const char* nombreFile, const char* tag, int nroPagina){
    pthread_mutex_lock(&memoria_mutex);

    int tam_pagina = configW->BLOCK_SIZE;
    if (nroPagina < 0 || nroPagina >= cant_paginas) {
        log_error(logger, "Acceso inválido a memoria: página %d", nroPagina);
        pthread_mutex_unlock(&memoria_mutex);
        return NULL; // Valor inválido
    }

    void* buffer = malloc(tam_pagina);
    if (buffer == NULL) {
        log_error(logger, "Error al asignar memoria para leer página %d", nroPagina);
        pthread_mutex_unlock(&memoria_mutex);
        return NULL;
    }

    memcpy(buffer, memoria + nroPagina * tam_pagina, tam_pagina);

    pthread_mutex_unlock(&memoria_mutex);
    return buffer;
}

void escribirEnMemoriaPaginaCompleta(const char* nombreFile, const char* tag, int nroPagina, void* contenidoPagina, size_t size){
    pthread_mutex_lock(&memoria_mutex);

    int tam_pagina = configW->BLOCK_SIZE;
    if (nroPagina < 0 || nroPagina >= cant_paginas || size > tam_pagina) {
        log_error(logger, "Acceso inválido a memoria: página %d, size %zu", nroPagina, size);
        pthread_mutex_unlock(&memoria_mutex);
        return;
    }

    memcpy(memoria + nroPagina * tam_pagina, contenidoPagina, size);

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
