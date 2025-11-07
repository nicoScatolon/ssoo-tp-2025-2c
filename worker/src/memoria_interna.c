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

// Obtiene la tabla de páginas para un <FILE>:<TAG>
TablaDePaginas* obtenerTablaPorFileYTag(char* nombreFile, char* tag){

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
        escribirEnMemoriaPaginaCompleta(nombreFile, tag, numeroPagina, marcoLibre, contenido, configW->BLOCK_SIZE);
        return marcoLibre;
        free(contenido);
    }
}

int obtenerMarcoDesdePagina(char* nombreFile, char* tag, int numeroPagina){
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

    enviarOpcode(READ_BLOCK, socketStorage);

    t_paquete* paquete = crearPaquete();
    agregarStringAPaquete(paquete, nombreFile);
    agregarStringAPaquete(paquete, tag);
    agregarIntAPaquete(paquete, numeroPagina);
    enviarPaquete(paquete, socketStorage);
    eliminarPaquete(paquete);

    char* contenido = escucharStorageContenidoPagina();

    return contenido;
}

int enviarPaginaAStorage(char* nombreFile, char* tag, int numeroPagina){
    char* contenido = obtenerContenidoDelMarco(obtenerNumeroDeMarco(nombreFile, tag, numeroPagina)); //esta bien usar obtener numero de marco aca?? Entendia q la usaba el query interpreter. a
    if (!contenido){
        log_error(logger, "Error al obtener el contenido del marco para enviar a Storage %s:%s pagina %d", nombreFile, tag, numeroPagina);
        return -1;
    }

    enviarOpcode(WRITE_BLOCK, socketStorage);
    t_paquete* paquete = crearPaquete();
    agregarStringAPaquete(paquete, nombreFile);
    agregarStringAPaquete(paquete, tag);
    agregarIntAPaquete(paquete, numeroPagina);
    agregarStringAPaquete(paquete, contenido);
    enviarPaquete(paquete, socketStorage);
    eliminarPaquete(paquete);
    free(contenido);
    log_debug(logger, "Envio a Storage del marco de %s:%s pagina %d", nombreFile, tag, numeroPagina);

    int respuesta = escucharStorage();
    if (respuesta == -1){
        log_error(logger, "Error al enviar pagina a Storage %s:%s pagina %d", nombreFile, tag, numeroPagina);
        return -1;
    }

    return 1;
}


// Lectura en "Memoria Interna"
char* leerContenidoDesdeOffset(char* nombreFile, char* tag, int numeroPagina, int numeroMarco, int offset, int size){
    
    pthread_mutex_lock(&memoria_mutex);

    char* contenido = obtenerContenidoDelMarco(numeroMarco);
    if(!contenido){
        pthread_mutex_unlock(&memoria_mutex);
        return NULL;
    }
    pthread_mutex_unlock(&memoria_mutex);

    pthread_mutex_lock(&tabla_paginas_mutex);
    TablaDePaginas* tabla = obtenerTablaPorFileYTag(nombreFile, tag);
    if(!tabla){
        log_error(logger, "Error al obtener tabla de paginas para %s:%s", nombreFile, tag);
        free(contenido);
        pthread_mutex_unlock(&tabla_paginas_mutex);
        return NULL;
    }

    EntradaDeTabla* entrada = &tabla->entradas[numeroPagina];
    actualizar_acceso_pagina(entrada);

    pthread_mutex_unlock(&tabla_paginas_mutex);

    
    return contenido;
}

//Escritura en "Memoria Interna"
//revisar si es necesario pasarle el size
void escribirContenidoDesdeOffset(char* nombreFile, char* tag, int numeroPagina, int numeroMarco, char* contenido, int offset, int size){
    if(offset + size > configW->BLOCK_SIZE){
        log_error(logger, "Error de escritura: offset %d + size %d excede el tamaño de la página %d", offset, size, configW->BLOCK_SIZE);
        exit(EXIT_FAILURE);
        return;
    }
    
    TablaDePaginas* tabla = obtenerTablaPorFileYTag(nombreFile, tag);
    if(tabla->keyProceso != (string_from_format("%s:%s", nombreFile, tag))){
        log_error(logger, "EL Marco %d NO pertenece al FILE:TAG %s:%s", numeroMarco, nombreFile, tag);
        return;
    }
    pthread_mutex_lock(&memoria_mutex);
    memcpy(memoria + (numeroMarco * configW->BLOCK_SIZE) + offset, contenido, size);
    pthread_mutex_unlock(&memoria_mutex);

    pthread_mutex_lock(&tabla_paginas_mutex);

    tabla->hayPaginasModificadas = true;
    EntradaDeTabla* entrada = &tabla->entradas[numeroPagina];

    modificar_pagina(entrada);

    pthread_mutex_unlock(&tabla_paginas_mutex);
}

//Debe escribir todo el contenido de un marco
void agregarContenidoAMarco(int numeroMarco, char* contenido){
    pthread_mutex_lock(&memoria_mutex);
    memcpy(memoria + (numeroMarco * configW->BLOCK_SIZE), contenido, configW->BLOCK_SIZE);
    pthread_mutex_unlock(&memoria_mutex);
}


void* leerDesdeMemoriaPaginaCompleta(char* nombreFile, char* tag, int numeroMarco){
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
void escribirEnMemoriaPaginaCompleta(char* nombreFile, char* tag, int numeroPagina, int marcoLibre, char* contenidoPagina, int size){
    // Validación de tamaño
    if (size > configW->BLOCK_SIZE) {
        log_error(logger, "Error: size %d excede BLOCK_SIZE %d", size, configW->BLOCK_SIZE);
        return;
    }
    
    // Validez del marco
    if(marcoLibre < 0 || marcoLibre >= cant_frames){
        log_error(logger, "Numero de marco %d invalido (max: %d)", marcoLibre, cant_frames - 1);
        return;
    }
    
    // Obtener o crear tabla 
    TablaDePaginas* tabla = obtenerTablaPorFileYTag(nombreFile, tag); // (CON lock interno)
    
    if(!tabla){
        // Crear nueva tabla
        agreagarTablaPorFileTagADicionario(nombreFile, tag); // (CON lock interno)
        tabla = obtenerTablaPorFileYTag(nombreFile, tag); // (CON lock interno)
        
        if(!tabla){
            log_error(logger, "Error al crear tabla de paginas para %s:%s", nombreFile, tag);
            return;
        }
    }
    
    // Ahora sí, hacer operaciones críticas con lock
    pthread_mutex_lock(&tabla_paginas_mutex);
    
    // Verificar validez del número de página
    if(numeroPagina < 0){
        log_error(logger, "Numero de pagina %d invalido %s:", numeroPagina, nombreFile, tag);
        pthread_mutex_unlock(&tabla_paginas_mutex);
        return;
    }
    
    // Configurar la entrada de la página
    EntradaDeTabla* entrada = &tabla->entradas[numeroPagina];
    
    // Si la página ya estaba presente en otro marco, liberar el marco anterior
    // if(entrada->bitPresencia && entrada->numeroFrame != marcoLibre){
    //     log_warning(logger, "Página %d ya estaba en marco %d, reemplazando con marco %d", 
    //                 numeroPagina, entrada->numeroFrame, marcoLibre);
    //     int marcoAnterior = entrada->numeroFrame;
    //     tabla->paginasPresentes--;
        
    //     pthread_mutex_unlock(&tabla_paginas_mutex);
    //     liberarMarco(marcoAnterior); // Tiene su propio lock
    //     pthread_mutex_lock(&tabla_paginas_mutex);
    // }
    
    // Si es una página nueva (no estaba presente)
    if(!entrada->bitPresencia){
        tabla->paginasPresentes++;
    }
    
    // Actualizar la entrada
    entrada->numeroFrame = marcoLibre;
    entrada->bitPresencia = true;
    
    pthread_mutex_unlock(&tabla_paginas_mutex);
    
    // Escribir en memoria física
    agregarContenidoAMarco(marcoLibre, contenidoPagina);
    
    // Actualizar metadatos de la página
    pthread_mutex_lock(&tabla_paginas_mutex);
    modificar_pagina(entrada);
    tabla->hayPaginasModificadas = true;
    pthread_mutex_unlock(&tabla_paginas_mutex);
    
    log_debug(logger, "Página %d de %s:%s escrita en marco %d", numeroPagina, nombreFile, tag, marcoLibre);
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


//Devuelve el marco liberado, donde se llame debe encargarse de ocupar el marco liberado
int ejecutarAlgoritmoReemplazo() {
    EntradaDeTabla* paginaAReemplazar = NULL;
    char* key = NULL; //file:Tag
    int marcoLiberado = -1;
    if (string_equals_ignore_case(configW->algoritmoReemplazo, "LRU")) {
       key = ReemplazoLRU(&paginaAReemplazar); 
    } else if (string_equals_ignore_case(configW->algoritmoReemplazo, "CLOCK-M")) {
        key = ReemplazoCLOCKM(&paginaAReemplazar);
    } else {
        log_error(logger, "Algoritmo de reemplazo desconocido: %s", configW->algoritmoReemplazo);
        return marcoLiberado; // Error: algoritmo desconocido
    }
    if (!paginaAReemplazar) {
        log_error(logger, "Error: paginaAReemplazar es NULL");
        free(key);
        return marcoLiberado;
    }

    char *fileName = NULL, *tagFile = NULL;
    
    if (!ObtenerNombreFileYTag(key, &fileName, &tagFile)) {
        log_error(logger, "Error al obtener <FILE>:<TAG> en ejecutarAlgoritmoReemplazo. key: %s", key);
        if (key) free(key);
        return marcoLiberado;
    }
    
    marcoLiberado = paginaAReemplazar->numeroFrame;
    
    int respuesta = enviarPaginaAStorage(fileName, tagFile, paginaAReemplazar->numeroPagina); // la funcion ya implementa la confirmacion de storage
    if (respuesta == -1){
        log_error(logger, "Error al enviar pagina a Storage %s:%s pagina %d", fileName, tagFile, paginaAReemplazar->numeroPagina);
        free(fileName);
        free(tagFile);
        free(paginaAReemplazar);
        free(key);
        return -1;
    }

    liberarMarco(marcoLiberado);

    free(fileName);
    free(tagFile);
    free(paginaAReemplazar);
    free(key);
    log_debug(logger, "Página %s reemplazada en marco %d", key, marcoLiberado);

    return marcoLiberado;
}

int64_t obtener_tiempo_actual() {
    t_temporal* temp = temporal_create();
    int64_t tiempo = temporal_gettime(temp);
    temporal_destroy(temp);
    return tiempo;
}

void inicializar_entrada(EntradaDeTabla* entrada, int numeroPagina) {
    entrada->numeroPagina = numeroPagina;
    entrada->numeroFrame = -1;  // Sin frame asignado aún
    entrada->ultimoAcceso = obtener_tiempo_actual();  // Timestamp de creación
    entrada->bitModificado = false;
    entrada->bitUso = false;
    entrada->bitPresencia = false;
}

// Al ACCEDER a una página (lectura o escritura)
void actualizar_acceso_pagina(EntradaDeTabla* entrada) {
    entrada->ultimoAcceso = obtener_tiempo_actual();  // Actualizar timestamp
    entrada->bitUso = true;
}

// Al MODIFICAR una página (escritura)
void modificar_pagina(EntradaDeTabla* entrada) {
    entrada->ultimoAcceso = obtener_tiempo_actual();  // Actualizar timestamp
    entrada->bitUso = true;
    entrada->bitModificado = true;
}


char* ReemplazoLRU(EntradaDeTabla** entradaAReemplazar) {
    char* keyProceso = NULL;
    EntradaDeTabla* entradaMenorTiempo = NULL;
    int64_t menorTiempo = INT64_MAX;  // Empezamos con el valor maximo posible

    pthread_mutex_lock(&tabla_paginas_mutex);
    
    t_list* keys = dictionary_keys(tablasDePaginas);
    
    for(int i = 0; i < list_size(keys); i++) {
        char* currentKey = list_get(keys, i);
        TablaDePaginas* tabla = dictionary_get(tablasDePaginas, currentKey);
        
        for(int j = 0; j < tabla->cantidadEntradasUsadas; j++) {
            EntradaDeTabla* entrada = &(tabla->entradas[j]);
            
            // Solo considerar páginas que están presentes en memoria
            if(entrada->bitPresencia) {
                // Comparación directa de timestamps
                if(entrada->ultimoAcceso < menorTiempo) {
                    menorTiempo = entrada->ultimoAcceso;
                    entradaMenorTiempo = entrada;
                    keyProceso = currentKey;
                }
            }
        }
    }
    
    list_destroy(keys);
    pthread_mutex_unlock(&tabla_paginas_mutex);
    
    // Asignar la entrada encontrada al puntero de salida
    if(entradaAReemplazar != NULL) {
        *entradaAReemplazar = entradaMenorTiempo;
    }
    
    return keyProceso;
}

char* ReemplazoCLOCKM(EntradaDeTabla** entradaAReemplazar) {
    char* key = NULL;
    EntradaDeTabla* entradaVictima = NULL;
    bool encontrado = false;

    pthread_mutex_lock(&tabla_paginas_mutex);
    
    t_list* keys = dictionary_keys(tablasDePaginas);
    int totalProcesos = list_size(keys);
    
    if(totalProcesos == 0) {
        list_destroy(keys);
        pthread_mutex_unlock(&tabla_paginas_mutex);
        return NULL;
    }
    
    // Inicializar puntero si es la primera vez
    if(punteroClockMod.keyProceso == NULL) {
        punteroClockMod.keyProceso = strdup(list_get(keys, 0));
        punteroClockMod.indicePagina = 0;
    }
    
    // PASO 1: Buscar (0,0) - bitUso=false, bitModificado=false
    encontrado = buscar_victima_clock(keys, &entradaVictima, &key, false, false, false);
    
    if(!encontrado) {
        // PASO 2: Buscar (0,1) - bitUso=false, bitModificado=true
        //         Durante la búsqueda, poner bitUso=false en todas las páginas que no coincidan
        encontrado = buscar_victima_clock(keys, &entradaVictima, &key, false, true, true);
        
        if(!encontrado) {
            // PASO 3: Volver a buscar (0,0) porque pusimos todos los bitUso en false
            encontrado = buscar_victima_clock(keys, &entradaVictima, &key, false, false, false);
        }
    }
    
    list_destroy(keys);
    pthread_mutex_unlock(&tabla_paginas_mutex);
    
    // Asignar la entrada encontrada al puntero de salida
    if(entradaAReemplazar != NULL) {
        *entradaAReemplazar = entradaVictima;
    }
    
    return key; // Devuelve la key del proceso cuya página será reemplazada
}


void limpiar_puntero_clockM() {
    if(punteroClockMod.keyProceso != NULL) {
        free(punteroClockMod.keyProceso);
        punteroClockMod.keyProceso = NULL;
    }
}

bool buscar_victima_clock(t_list* keys, EntradaDeTabla** victima, char** keyOut, 
                          bool buscarBitUso, bool buscarBitMod, bool limpiarBitUso) {
    int totalProcesos = list_size(keys);
    int procesoActual = encontrar_indice_proceso(keys, punteroClockMod.keyProceso);
    int paginaActual = punteroClockMod.indicePagina;
    
    int paginasRevisadas = 0;
    int totalPaginasEnMemoria = contar_paginas_presentes(keys);
    
    if(totalPaginasEnMemoria == 0) {
        return false;
    }
    
    while(paginasRevisadas < totalPaginasEnMemoria) {
        char* currentKey = list_get(keys, procesoActual);
        TablaDePaginas* tabla = dictionary_get(tablasDePaginas, currentKey);
        
        for(int j = paginaActual; j < tabla-> cantidadEntradasUsadas; j++) { // reemplace capacidadEntradas a cantidadEntradasUsadas
            EntradaDeTabla* entrada = &(tabla->entradas[j]);
            
            if(entrada->bitPresencia) {
                paginasRevisadas++;
                
                // Verificar si cumple el criterio buscado
                if(entrada->bitUso == buscarBitUso && 
                   entrada->bitModificado == buscarBitMod) {
                    // ¡Encontramos la víctima!
                    *victima = entrada;
                    *keyOut = currentKey;
                    
                    // Avanzar el puntero para la próxima vez
                    punteroClockMod.indicePagina = j + 1;
                    
                    if(punteroClockMod.indicePagina >= tabla->cantidadEntradasUsadas) {
                        // Si llegamos al final de esta tabla, pasar al siguiente proceso
                        procesoActual = (procesoActual + 1) % totalProcesos;
                        punteroClockMod.indicePagina = 0;
                        
                        if(punteroClockMod.keyProceso != NULL) {
                            free(punteroClockMod.keyProceso);
                        }
                        punteroClockMod.keyProceso = strdup(list_get(keys, procesoActual));
                    } else {
                        if(punteroClockMod.keyProceso != NULL) {
                            free(punteroClockMod.keyProceso);
                        }
                        punteroClockMod.keyProceso = strdup(currentKey);
                    }
                    
                    return true;
                }
                
                // Si debemos limpiar el bit de uso y no encontramos víctima
                if(limpiarBitUso) {
                    entrada->bitUso = false;
                }
            }
        }
        
        // Avanzar al siguiente proceso
        procesoActual = (procesoActual + 1) % totalProcesos;
        paginaActual = 0;
        
        // Actualizar el puntero del reloj
        if(punteroClockMod.keyProceso != NULL) {
            free(punteroClockMod.keyProceso);
        }
        punteroClockMod.keyProceso = strdup(list_get(keys, procesoActual));
        punteroClockMod.indicePagina = 0;
    }
    
    return false;
}

int encontrar_indice_proceso(t_list* keys, char* keyBuscada) {
    for(int i = 0; i < list_size(keys); i++) {
        if(strcmp(list_get(keys, i), keyBuscada) == 0) {
            return i;
        }
    }
    return 0;
}

int contar_paginas_presentes(t_list* keys) {
    int count = 0;
    for(int i = 0; i < list_size(keys); i++) {
        char* currentKey = list_get(keys, i);
        TablaDePaginas* tabla = dictionary_get(tablasDePaginas, currentKey);
        count += tabla->paginasPresentes;
    }
    return count;
}