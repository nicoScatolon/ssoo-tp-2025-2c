#include "memoria_interna.h"

void asignarCant_paginas(void){
    cant_paginas = configW->tamMemoria / configW->BLOCK_SIZE;
}


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

    char* key = string_from_format("%s:%s", nombreFile, tag);
    if (!key) {
        log_error(logger, "Sin memoria para crear la clave file:tag");
        return NULL;
    }

    pthread_mutex_lock(&tabla_paginas_mutex);
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

//Reserva y libera marcos en el bitmap
int obtenerMarcoLibre(void){
    pthread_mutex_lock(&memoria_mutex);

    for (int i = 0; i < cant_paginas; i++) {
        if (!bitarray_test_bit(bitmap, i)) { // Página libre
            bitarray_set_bit(bitmap, i); // Marcar como usada
            pthread_mutex_unlock(&memoria_mutex);
            return i; // Retornar número de página libre
        }
    }
    pthread_mutex_unlock(&memoria_mutex);
    log_debug(logger, "No hay marcos libres disponibles");
    int marco = ejecutarAlgoritmoReemplazo();
    if(marco < 0){
        log_error(logger, "Error al ejecutar el algoritmo de reemplazo");
        return -1;
    }
    return marco; // No hay páginas libres
}

//poner el bit del bitmap en 0
void liberarMarco(int nro_marco){
    pthread_mutex_lock(&memoria_mutex);

    if(!bitarray_test_bit(bitmap, nro_marco)){ //si el marco NO esta ocupado tira error
        log_warning(logger, "El marco %d ya está libre", nro_marco);
        pthread_mutex_unlock(&memoria_mutex);
        return;
    }
    bitarray_clean_bit(bitmap, nro_marco);

    pthread_mutex_unlock(&memoria_mutex);
}

//Devuelve el contenido todo del marco pedido. Me traigo el contenido de un marco, para leer, escribir, enviarlo a storage.
char* obtenerContenidoDelMarco(int nro_marco){
    if (nro_marco < 0) return NULL;

    size_t blockSize = (size_t) configW->BLOCK_SIZE;
    size_t bitInicialMarco = (size_t)nro_marco * blockSize;

    pthread_mutex_lock(&memoria_mutex);
    /* string_substring hace malloc y copia 'size' bytes desde la posición indicada */
    char *contenido = string_substring(memoria + bitInicialMarco, 0, blockSize);
    pthread_mutex_unlock(&memoria_mutex);

    if (!contenido) {
        log_error(logger, "Error al obtener el contenido del marco %d (malloc fallo)", nro_marco);
        return NULL;
    }

    return contenido; // caller debe free(contenido)
}

//general a usar por query_interpreter
int obtenerNumeroDeMarco(char* nombreFile, char* tag, int numeroPagina){
    if(obtenerMarcoDesdePagina(nombreFile, tag, numeroPagina) != -1){
        return obtenerMarcoDesdePagina(nombreFile, tag, numeroPagina);
    }
    else{
        char* contenido = traerPaginaDeStorage(nombreFile, tag, numeroPagina);
        if(!contenido){
            log_error(logger, "Error al traer pagina de Storage %s:%s pagina %d", nombreFile, tag, numeroPagina);
            return -1;
        }
        int marcoLibre = obtenerMarcoLibre();
        if(marcoLibre == -1){
            log_error(logger, "Error al obtener marco libre para cargar pagina %s:%s pagina %d", nombreFile, tag, numeroPagina);
            free(contenido);
            return -1;
        }
        escribirEnMemoriaPaginaCompleta(nombreFile, tag, marcoLibre, contenido, configW->BLOCK_SIZE);
        free(contenido);
        return marcoLibre;
    }
}

//Por hacer
void agregarContenidoAMarco(char* nombreFile, char* tag, int numeroPagina, char* contenido){

}

int obtenerMarcoDesdePagina(const char* nombreFile, const char* tag, int numeroPagina){

    TablaDePaginas* tabla =  obtenerTablaPorFileYTag(nombreFile, tag);
    pthread_mutex_lock(&tabla_paginas_mutex);
    if (!tabla) {
        pthread_mutex_lock(&tabla_paginas_mutex);
        return -1;
    }
    if(tabla->paginasPresentes <= 0){
        log_debug(logger, "No hay paginas presentes en la tabla de paginas de %s:%s", nombreFile, tag);
        pthread_mutex_lock(&tabla_paginas_mutex);
        return -1;
    }
    if(tabla->entradas[numeroPagina].bitPresencia == false){
        log_debug(logger, "La pagina %d no está en memoria para el proceso %s:%s", numeroPagina, nombreFile, tag);
        pthread_mutex_lock(&tabla_paginas_mutex);
        return -1;
    }
    int marco = tabla->entradas[numeroPagina].numeroFrame;
    pthread_mutex_lock(&tabla_paginas_mutex);

    return marco;
}


char* traerPaginaDeStorage(char* nombreFile, char* tag, int numeroPagina){

    enviarOpcode(READ_BLOCK, socketStorage/*socket storage*/);

    t_paquete* paquete = crearPaquete();
    agregarStringAPaquete(paquete, nombreFile);
    agregarStringAPaquete(paquete, tag);
    agregarIntAPaquete(paquete, numeroPagina);
    enviarPaquete(paquete, socketStorage/*socket storage*/);
    eliminarPaquete(paquete);

    char* contenido = escucharStorageContenidoPagina();

    return contenido;
}

void enviarPaginaAStorage(char* nombreFile, char* tag, int numeroPagina){
    
    char* contenido = obtenerContenidoDelMarco(obtenerNumeroDeMarco(nombreFile, tag, numeroPagina));
    if (!contenido){
        log_error(logger, "Error al obtener el contenido del marco para enviar a Storage %s:%s pagina %d", nombreFile, tag, numeroPagina);
        return;
    }

    enviarOpcode(WRITE_BLOCK, socketStorage/*socket storage*/);
    t_paquete* paquete = crearPaquete();
    agregarStringAPaquete(paquete, nombreFile);
    agregarStringAPaquete(paquete, tag);
    agregarIntAPaquete(paquete, numeroPagina);
    agregarStringAPaquete(paquete, contenido);
    enviarPaquete(paquete, socketStorage/*socket storage*/);
    eliminarPaquete(paquete);
    free(contenido);
    log_debug(logger, "Envio a Storage del marco de %s:%s pagina %d", nombreFile, tag, numeroPagina);


    // a charlar, ver si necesito confirmacion.

}



// Lectura en "Memoria Interna"
char* leerContenidoDesdeOffset(const char* nombreFile, const char* tag, int numeroMarco, int offset, int size){
    pthread_mutex_lock(&memoria_mutex);

    char* contenido = obtenerContenidoDelMarco(numeroMarco);
    if(!contenido){
        pthread_mutex_unlock(&memoria_mutex);
        return NULL;
    }
    

    pthread_mutex_unlock(&memoria_mutex);
    return NULL;
}

//Escritura en "Memoria Interna"
void escribirContenidoDesdeOffset(const char* nombreFile, const char* tag, int numeroMarco, char* contenido, int offset, int size){
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

//tiene que revisar si el file:tag ya tiene la pagina en la tabla de paginas. o revisar si la pagina esta en la tabla de paginas (aunque sea ausente)
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




//---No se si es necesario---
// int obtenerNumeroPaginaDeFileTag(const char* nombreFile, const char* tag, int direccionBase){
//     pthread_mutex_lock(&tabla_paginas_mutex);

//     // Puede llamar a obtenerTablaPorFileYTag(nombreFile, tag) para obtener la tabla de páginas de ese file:tag

//     // Aquí deberías implementar la lógica para buscar la página correspondiente
//     // al <FILE>:<TAG> y la dirección base. Esto implica buscar en la tabla de páginas del proceso y calcular el número de página basado en la dirección base.

    

//     pthread_mutex_unlock(&tabla_paginas_mutex);
//     return -1; // Retornar el número de página correspondiente o -1 si no se encuentra
// }



int ejecutarAlgoritmoReemplazo() {
    int paginaAReemplazar;
    int marcoLiberado;
    if (string_equals_ignore_case(configW->algoritmoReemplazo, "LRU")) {
       paginaAReemplazar = ReemplazoLRU(); 
    } else if (string_equals_ignore_case(configW->algoritmoReemplazo, "CLOCK-M")) {
        paginaAReemplazar = ReemplazoCLOCKM();
    } else {
        log_error(logger, "Algoritmo de reemplazo desconocido: %s", configW->algoritmoReemplazo);
        return -1; // Error: algoritmo desconocido
    }

    //implementar el reemplazo
    //mandar paginaAReemplazar a storage
    //devolver marco liberado

    return marcoLiberado;
}



int ReemplazoLRU() {
    // Implementación del algoritmo de reemplazo LRU
    return -1; // Devuelve la pagina a ser reemplazada
}

int ReemplazoCLOCKM() {
    // Implementación del algoritmo de reemplazo CLOCK-M
    return -1; // Devuelve la pagina a ser reemplazada
}
