#include <memoria_interna.h>


void inicializarMemoriaInterna(void) {
    pthread_mutex_lock(&memoria_mutex);

    if (memoria) { // ya inicializada
        pthread_mutex_unlock(&memoria_mutex);
        return;
    }

    int tam_memoria = configW->tamMemoria;
    int tam_frame  = configW->BLOCK_SIZE;
    if (tam_frame <= 0) {
        log_error(logger, "BLOCK_SIZE invalido: %d", tam_frame);
        pthread_mutex_unlock(&memoria_mutex);
        exit(EXIT_FAILURE);
    }

    cant_frames = tam_memoria / tam_frame;
    if (cant_frames <= 0) {
        log_error(logger, "Tam memoria insuficiente para pagina de %d bytes", tam_frame);
        pthread_mutex_unlock(&memoria_mutex);
        exit(EXIT_FAILURE);
    }

    memoria = calloc(1, tam_memoria);
    if (memoria == NULL) {
        log_error(logger, "Error al asignar memoria interna de tamaño %d", tam_memoria);
        pthread_mutex_unlock(&memoria_mutex);
        exit(EXIT_FAILURE);
    }

    int bytes_bitmap = (cant_frames + 7) / 8; // redondeo hacia arriba
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
              tam_memoria, cant_frames, bytes_bitmap);

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



void agreagarTablaPorFileTagADicionario(char* nombreFile, char* tag){ 
    pthread_mutex_lock(&tabla_paginas_mutex);

    char* key = string_from_format("%s:%s", nombreFile, tag);
    if (!key) {
        log_error(logger, "Sin memoria para clave %s", key);
        pthread_mutex_unlock(&tabla_paginas_mutex);
        return;
    }

    if (dictionary_has_key(tablasDePaginas, key)) {
        log_warning(logger, "La tabla de paginas para el proceso %s ya existe", key);
        free(key);
        pthread_mutex_unlock(&tabla_paginas_mutex);
        return;
    }

     TablaDePaginas* tabla = calloc(1, sizeof(TablaDePaginas));
    if (!tabla) {
        log_error(logger, "Error al asignar memoria para la tabla de paginas del proceso %s", key);
        free(key);
        pthread_mutex_unlock(&tabla_paginas_mutex);
        return;
    }

    dictionary_put(tablasDePaginas, key, tabla);
    log_debug(logger, "Tabla de paginas creada para el proceso %s", key);

    free(key);
    pthread_mutex_unlock(&tabla_paginas_mutex);

    return;
}

TablaDePaginas* obtenerTablaPorFileYTag(const char* nombreFile, const char* tag){
    pthread_mutex_lock(&tabla_paginas_mutex);

    char* key = string_from_format("%s:%s", nombreFile, tag);
    if (!key) {
        log_error(logger, "Sin memoria para crear la clave file:tag");
        pthread_mutex_unlock(&tabla_paginas_mutex);
        return NULL;
    }

    TablaDePaginas* tabla = dictionary_get(tablasDePaginas, key);
    if (!tabla) {
        log_warning(logger, "No se encontró tabla de páginas para %s", key);
    }

    free(key);
    pthread_mutex_unlock(&tabla_paginas_mutex);
    return tabla;
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

    cant_frames = 0;

    pthread_mutex_unlock(&memoria_mutex);
}

int obtenerMarcoLibre(void){
    pthread_mutex_lock(&memoria_mutex);

    // for (int i = 0; i < cant_paginas; i++) {
    //     if (!bitarray_test_bit(bitmap, i)) { // Página libre
    //         bitarray_set_bit(bitmap, i); // Marcar como usada
    //         pthread_mutex_unlock(&memoria_mutex);
    //         return i; // Retornar número de página libre
    //     }
    // }

    pthread_mutex_unlock(&memoria_mutex);
    return -1; // No hay páginas libres
}

void liberarMarco(int nro_marco){
    pthread_mutex_lock(&memoria_mutex);


    pthread_mutex_unlock(&memoria_mutex);
}

void reservarMarco(int nro_marco){
    pthread_mutex_lock(&memoria_mutex);



    pthread_mutex_unlock(&memoria_mutex);
}


// Lectura/Escritura en "Memoria Interna"
char* leerContenidoDesdeOffset(const char* nombreFile, const char* tag, int numeroMarco, int size, int offset){
    pthread_mutex_lock(&memoria_mutex);


    pthread_mutex_unlock(&memoria_mutex);}

void escribirContenidoDesdeOffset(const char* nombreFile, const char* tag, int numeroMarco, int offset, int size, char* contenido){
    pthread_mutex_lock(&memoria_mutex);


    pthread_mutex_unlock(&memoria_mutex);
}

void* leerDesdeMemoriaPaginaCompleta(const char* nombreFile, const char* tag, int numeroMarco){
    // pthread_mutex_lock(&memoria_mutex);

    // int tam_pagina = configW->BLOCK_SIZE;
    // if (nroPagina < 0 || nroPagina >= cant_paginas) {
    //     log_error(logger, "Acceso inválido a memoria: página %d", nroPagina);
    //     pthread_mutex_unlock(&memoria_mutex);
    //     return NULL; // Valor inválido
    // }

    // void* buffer = malloc(tam_pagina);
    // if (buffer == NULL) {
    //     log_error(logger, "Error al asignar memoria para leer página %d", nroPagina);
    //     pthread_mutex_unlock(&memoria_mutex);
    //     return NULL;
    // }

    // memcpy(buffer, memoria + nroPagina * tam_pagina, tam_pagina);

    // pthread_mutex_unlock(&memoria_mutex);
    // return buffer;
}

void escribirEnMemoriaPaginaCompleta(const char* nombreFile, const char* tag, int numeroMarco, char* contenidoPagina, int size){
    // pthread_mutex_lock(&memoria_mutex);

    // int tam_pagina = configW->BLOCK_SIZE;
    // if (nroPagina < 0 || nroPagina >= cant_paginas || size > tam_pagina) {
    //     log_error(logger, "Acceso inválido a memoria: página %d, size %zu", nroPagina, size);
    //     pthread_mutex_unlock(&memoria_mutex);
    //     return;
    // }

    // memcpy(memoria + nroPagina * tam_pagina, contenidoPagina, size);

    // pthread_mutex_unlock(&memoria_mutex);
}

int obtenerNumeroPaginaDeFileTag(const char* nombreFile, const char* tag, int direccionBase){
    pthread_mutex_lock(&tabla_paginas_mutex);

    // Puede llamar a obtenerTablaPorFileYTag(nombreFile, tag) para obtener la tabla de páginas de ese file:tag

    // Aquí deberías implementar la lógica para buscar la página correspondiente
    // al <FILE>:<TAG> y la dirección base. Esto implica buscar en la tabla de páginas del proceso y calcular el número de página basado en la dirección base.

    

    pthread_mutex_unlock(&tabla_paginas_mutex);
    return -1; // Retornar el número de página correspondiente o -1 si no se encuentra
}

int pedirMarco(const char* nombreFile, const char* tag, int numeroPagina){
    // pthread_mutex_lock(&tabla_paginas_mutex);

    // Aquí deberías implementar la lógica para pedir una página al Storage
    // y actualizar la tabla de páginas del proceso correspondiente.

    // pthread_mutex_unlock(&tabla_paginas_mutex);
    return -1; // Retornar el número de página asignada o -1 en caso de error
}






int ReemplazoLRU() {
    // Implementación del algoritmo de reemplazo LRU
    return -1; // Devuelve la página reemplazada (ejemplo)
}

int ReemplazoCLOCKM() {
    // Implementación del algoritmo de reemplazo CLOCK-M
    return -1; // Devuelve la página reemplazada (ejemplo)
}
