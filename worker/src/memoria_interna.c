#include "memoria_interna.h"

char *memoria = NULL;        // memoria principal
t_bitarray* bitmap = NULL;   // bitmap para páginas
int cant_marcos = 0;
t_dictionary* tablasDePaginas = NULL; //la key es <FILE>:<TAG>

t_temporal* temp = NULL;

//Mutex
pthread_mutex_t memoria_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t tabla_paginas_mutex = PTHREAD_MUTEX_INITIALIZER;

void aplicarRetardoMemoria() {
    log_debug(logger, "Retardo de memoria aplicado: %d ms", configW->retardoMemoria);
    usleep(configW->retardoMemoria * 1000);
}


void inicializarMemoriaInterna(void) {
    pthread_mutex_lock(&memoria_mutex);

    if (memoria) { // ya inicializada
        pthread_mutex_unlock(&memoria_mutex);
        return;
    }

    int tam_memoria = configW->tamMemoria;
    int tam_marco  = configW->BLOCK_SIZE;
    if (tam_marco <= 0) {
        log_error(logger, "BLOCK_SIZE invalido: %d", tam_marco);
        pthread_mutex_unlock(&memoria_mutex);
        exit(EXIT_FAILURE);
    }

    temp = temporal_create();
    log_debug(logger, "Inicializando contador de actualizacion tablas de paginas");

    
    cant_marcos = tam_memoria / tam_marco;
    log_error(logger, "Tamaño de memoria: %d bytes", tam_memoria); //despues borrar no es un error real
    log_error(logger, "Tamaño de marco (BLOCK_SIZE): %d bytes", tam_marco); //despues borrar no es un error real
    log_error(logger, "Cantidad de marcos calculada: %d", cant_marcos); //despues borrar no es un error real

    if (cant_marcos <= 0) {
        log_error(logger, "Tam memoria insuficiente para pagina de %d bytes", tam_marco);
        pthread_mutex_unlock(&memoria_mutex);
        exit(EXIT_FAILURE);
    }

    memoria = calloc(1, tam_memoria);
    if (memoria == NULL) {
        log_error(logger, "Error al asignar memoria interna de tamaño %d", tam_memoria);
        pthread_mutex_unlock(&memoria_mutex);
        exit(EXIT_FAILURE);
    }

    int bytes_bitmap = (cant_marcos + CHAR_BIT - 1) / CHAR_BIT; // redondeo hacia arriba
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

    log_debug(logger, "Memoria interna inicializada: %d bytes, %d marcos, bitmap de %d bytes",
              tam_memoria, cant_marcos, bytes_bitmap);

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

// Inicializa y agrega una tabla al diccionario
TablaDePaginas* agreagarTablaPorFileTagADicionario(char* nombreFile, char* tag){ 
    pthread_mutex_lock(&tabla_paginas_mutex);

    char* key = string_from_format("%s:%s", nombreFile, tag);
    if (!key) {
        log_error(logger, "Sin memoria para clave");
        pthread_mutex_unlock(&tabla_paginas_mutex);
        return NULL;
    }

    if (dictionary_has_key(tablasDePaginas, key)) {
        log_warning(logger, "La tabla de paginas para %s ya existe", key);
        free(key);
        pthread_mutex_unlock(&tabla_paginas_mutex);
        return NULL;
    }

    TablaDePaginas* tabla = calloc(1, sizeof(TablaDePaginas));
    if (!tabla) {
        log_error(logger, "Error al asignar memoria para la tabla de paginas de %s", key);
        free(key);
        pthread_mutex_unlock(&tabla_paginas_mutex);
        return NULL;
    }

    tabla->keyProceso = strdup(key);
    log_debug(logger, "Creando tabla de paginas para %s, lo guardado en tabla->keyProceso: %s", key, tabla->keyProceso);
    if (!tabla->keyProceso) {
        log_error(logger, "Error al duplicar key para tabla");
        free(tabla->keyProceso);
        free(tabla);
        free(key);
        pthread_mutex_unlock(&tabla_paginas_mutex);
        return NULL;
    }

    int maxPaginas = (configW->FS_SIZE / configW->BLOCK_SIZE);
    if (maxPaginas <= 0) {
        log_error(logger, "Error: maxPaginas calculado es %d", maxPaginas);
        free(tabla->keyProceso);
        free(tabla);
        free(key);
        pthread_mutex_unlock(&tabla_paginas_mutex);
        return NULL;
    }

    tabla->entradas = calloc(maxPaginas, sizeof(EntradaDeTabla));
    if (!tabla->entradas) {
        log_error(logger, "Error al asignar memoria para entradas de %s", key);
        free(tabla->keyProceso);
        free(tabla);
        free(key);
        pthread_mutex_unlock(&tabla_paginas_mutex);
        return NULL;
    }

    for (int i = 0; i < maxPaginas; i++) {
        inicializar_entrada(&tabla->entradas[i], i);
    }

    tabla->capacidadEntradas = maxPaginas;
    tabla->paginasPresentes = 0;
    tabla->hayPaginasModificadas = false;

    // Agregar al diccionario
    dictionary_put(tablasDePaginas, key, tabla);
    
    log_debug(logger, "Tabla de paginas creada para %s con capacidad de %d páginas", 
              tabla->keyProceso, maxPaginas);

    free(key);
    pthread_mutex_unlock(&tabla_paginas_mutex);

    return tabla;
}

void actualizarMetadataTablaPagina(TablaDePaginas* tabla){
    if (!tabla) {
        log_error(logger, "Error: tabla NULL al actualizar metadata");
        return;
    }
    
    pthread_mutex_lock(&tabla_paginas_mutex);
    
    // Resetear contadores
    int paginasPresentes = 0;
    bool hayPaginasModificadas = false;
    
    // Recorrer todas las entradas de la tabla
    for (int i = 0; i < tabla->capacidadEntradas; i++) {
        EntradaDeTabla* entrada = &tabla->entradas[i];
        
        // Contar paginas presentes
        if (entrada->bitPresencia) {
            paginasPresentes++;
            
            // Verificar si hay paginas modificadas
            if (entrada->bitModificado) {
                hayPaginasModificadas = true;
            }
        }
    
    }
    
    // Actualizar metadata de la tabla
    tabla->paginasPresentes = paginasPresentes;
    tabla->hayPaginasModificadas = hayPaginasModificadas;
    
    pthread_mutex_unlock(&tabla_paginas_mutex);
    
    log_debug(logger, "Metadata actualizada para %s: %d páginas presentes, %s páginas modificadas",
              tabla->keyProceso, 
              tabla->paginasPresentes,
              tabla->hayPaginasModificadas ? "hay" : "no hay");
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
    if(tabla == NULL){
        log_debug(logger, "Entro aca, No existe tabla de paginas para %s:%s", nombreFile, tag);
        free(key);
        pthread_mutex_unlock(&tabla_paginas_mutex);
        return NULL;
    }
    //Se hace la validacion si no existe afuera con un log warning
    pthread_mutex_unlock(&tabla_paginas_mutex);
    free(key);
    log_debug(logger, "Tabla de paginas obtenida para %s:%s", nombreFile, tag);
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

    temporal_destroy(temp);

    pthread_mutex_lock(&tabla_paginas_mutex);
    if (tablasDePaginas) {
        dictionary_destroy_and_destroy_elements(tablasDePaginas, free);
        tablasDePaginas = NULL;
    }
    pthread_mutex_unlock(&tabla_paginas_mutex);

    cant_marcos = 0;

    pthread_mutex_unlock(&memoria_mutex);
}

//Si hay un marco libre, lo devuelve reservado. Si no hay, ejecuta el algoritmo de reemplazo y devuelve un marco libre y reservado.
int obtenerMarcoLibreYReservado(char* keyAsignar, int numeroPagina){
    pthread_mutex_lock(&memoria_mutex);

    for (int i = 0; i < cant_marcos; i++) {
        if (!bitarray_test_bit(bitmap, i)) {
            bitarray_set_bit(bitmap, i); 
            pthread_mutex_unlock(&memoria_mutex);

            char* tag = NULL;
            char* nombreFile = NULL;
            ObtenerNombreFileYTag(keyAsignar, &nombreFile, &tag);
            asignarMarcoEntradaTabla(nombreFile, tag, numeroPagina, i);
           
            log_debug(logger, "Marco libre encontrado: %d", i);

            free(nombreFile);
            free(tag);
            return i; // Retornar número de marco libre
        }
    }
    
    pthread_mutex_unlock(&memoria_mutex);
    log_debug(logger, "No hay marcos libres disponibles, se ejecuta el algoritmo de reemplazo");
    key_Reemplazo* keyVictima = ejecutarAlgoritmoReemplazo(); //deberia devolver file:tag (key), nro pagina y nro marco
    if(!keyVictima){
        log_error(logger, "Error al ejecutar el algoritmo de reemplazo");
        exit (EXIT_FAILURE);
        return -1;
    }
    int marco = liberarMarcoVictimaYAsignar(keyVictima, keyAsignar, numeroPagina);

    if (marco < 0){
        log_error(logger, "Error al obtener marco liberado y reservar para keyVictima: %s", keyVictima->key);
        free(keyVictima->key);
        free(keyVictima);
        exit(EXIT_FAILURE);
        
        return -1;
    }

    ocuparMarco(marco);

    free(keyVictima->key);
    free(keyVictima);
    return marco; 
}

// devuelve un marco liberado por la victima y reservado para el nuevo proceso
int liberarMarcoVictimaYAsignar(key_Reemplazo* keyVictima, char* keyAsignar, int numeroPagina){
    char* tagFileVictima = NULL;
    char* nombreFileVictima = NULL;
    
    if (!ObtenerNombreFileYTag(keyVictima->key, &nombreFileVictima, &tagFileVictima)) {
        log_error(logger, "Error al obtener <FILE>:<TAG> en liberarMarcoVictimaYAsignar. key: %s", keyVictima->key);
        exit(EXIT_FAILURE);
    }

    
    char* fileNameAsignar = NULL;
    char* tagFileAsignar = NULL;
    EntradaDeTabla* paginaAsignar = NULL;
    if (!ObtenerNombreFileYTag(keyAsignar, &fileNameAsignar, &tagFileAsignar)) {
        log_error(logger, "Error al obtener <FILE>:<TAG> en liberarMarcoVictimaYAsignar. key: %s", keyVictima->key);
        exit(EXIT_FAILURE);
    }
    int marcoLiberado = keyVictima->marco;

    

    int respuesta = enviarPaginaAStorage(nombreFileVictima, tagFileVictima, keyVictima->pagina); // la funcion ya implementa la confirmacion de storage
    if (respuesta == -1){
        log_error(logger, "Error al enviar pagina a Storage %s:%s pagina %d", nombreFileVictima, tagFileVictima, keyVictima->pagina);
        free(nombreFileVictima);
        free(tagFileVictima);
        free(fileNameAsignar);
        free(tagFileAsignar);
        return -1;
    } else if (respuesta == 1){
        TablaDePaginas* tabla = obtenerTablaPorFileYTag(nombreFileVictima, tagFileVictima); 
        tabla->entradas[keyVictima->pagina].bitPresencia = false; 
        tabla->entradas[keyVictima->pagina].bitModificado = false; 
    }

    log_info(logger,"“Query <%d>: Se libera el Marco: <%d> perteneciente al - File: <%s> - Tag: <%s>",contexto->query_id,marcoLiberado,nombreFileVictima,tagFileVictima);
    liberarMarco(marcoLiberado);

    asignarMarcoEntradaTabla(fileNameAsignar, tagFileAsignar, numeroPagina, marcoLiberado);

    log_info(logger,"## Query <%d>: Se reemplaza la página <%s:%s>/<%d> por la <%s:%s><%d>",
                                                                                            contexto->query_id,
                                                                                            nombreFileVictima,
                                                                                            tagFileVictima,
                                                                                            keyVictima->pagina, 
                                                                                            fileNameAsignar,
                                                                                            tagFileAsignar,
                                                                                            numeroPagina);

    free(nombreFileVictima);
    free(tagFileVictima);
    free(fileNameAsignar);
    free(tagFileAsignar);

    return marcoLiberado;
}

void asignarMarcoEntradaTabla(char* nombreFile, char* tag, int numeroPagina, int numeroMarco){
    TablaDePaginas* tabla = obtenerTablaPorFileYTag(nombreFile, tag); // (CON lock interno)
    if(!tabla){
        // Crear nueva tabla
        tabla = agreagarTablaPorFileTagADicionario(nombreFile, tag); // (CON lock interno)
        log_debug(logger, "Tabla de paginas creada para %s:%s al asignar marco", nombreFile, tag);
        
        if(!tabla){
            log_error(logger, "Error al crear tabla de paginas para %s:%s", nombreFile, tag);
            exit(EXIT_FAILURE);
        }
    }

    // Ahora sí, hacer operaciones críticas con lock
    pthread_mutex_lock(&tabla_paginas_mutex);
    
    if (!tabla->entradas) {
        log_error(logger, "ERROR CRÍTICO: tabla->entradas es NULL para %s:%s", nombreFile, tag);
        pthread_mutex_unlock(&tabla_paginas_mutex);
        exit(EXIT_FAILURE);
    }
    
    // Verificar validez del número de página
    if (numeroPagina < 0) {
        log_error(logger, "Numero de pagina %d invalido para %s:%s (max: %d)",numeroPagina, nombreFile, tag, tabla->capacidadEntradas - 1);
        pthread_mutex_unlock(&tabla_paginas_mutex);
        exit(EXIT_FAILURE);
    }
    
    // Configurar la entrada de la página
    log_debug(logger, "Capacidad de entradas de tabla: %d, Presentes: %d", tabla->capacidadEntradas, tabla->paginasPresentes);
    EntradaDeTabla* entrada = &tabla->entradas[numeroPagina];

    // Actualizar la entrada
    entrada->numeroMarco = numeroMarco;
    entrada->numeroPagina = numeroPagina;
    entrada->bitPresencia = true;
    entrada->ultimoAcceso = obtener_tiempo_actual();
    entrada->bitUso = true; //revisar si esta correcto...

    pthread_mutex_unlock(&tabla_paginas_mutex);

    actualizarMetadataTablaPagina(tabla);
    
    return;
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

void ocuparMarco(int nro_marco){
    pthread_mutex_lock(&memoria_mutex);

    if(bitarray_test_bit(bitmap, nro_marco)){ //si el marco ya esta ocupado tira error
        log_warning(logger, "El marco %d ya está ocupado", nro_marco);
        pthread_mutex_unlock(&memoria_mutex);
        return;
    }
    bitarray_set_bit(bitmap, nro_marco);

    pthread_mutex_unlock(&memoria_mutex);
}

//Devuelve el contenido todo del marco pedido. Me traigo el contenido de un marco, para leer, escribir, enviarlo a storage.
char* obtenerContenidoDelMarco(int nro_marco, int offset, int size){ //El limite es BlockSize o BlockSize-1??
    if (nro_marco < 0) return NULL;

    // if(offset < 0 || size < 0 || offset + size < configW->BLOCK_SIZE){
    //     log_error(logger, "Error de lectura: offset %d + size %d excede el tamaño de la página %d", offset, size, configW->BLOCK_SIZE);
    //     return NULL;
    // }

    size_t bitInicialMarco = (size_t)nro_marco * configW->BLOCK_SIZE + (offset);

    pthread_mutex_lock(&memoria_mutex);
    /* string_substring hace malloc y copia 'size' bytes desde la posición indicada */
    char *contenido = string_substring(memoria + bitInicialMarco, 0, size);
    pthread_mutex_unlock(&memoria_mutex);

    if (!contenido) {
        log_error(logger, "Error al obtener el contenido del marco %d (malloc fallo)", nro_marco);
        return NULL;
    }
    
    //SUMAR RETARDO DE MEMORIA (marco accedido)
    // log_info(logger,"“Query <QUERY_ID>: Acción: <LEER / ESCRIBIR> - Dirección Física: <DIRECCION_FISICA> - Valor: <VALOR LEIDO / ESCRITO>");
    aplicarRetardoMemoria();
    return contenido; // caller debe free(contenido)
}


//general a usar por query_interpreter
int obtenerNumeroDeMarco(char* nombreFile, char* tag, int numeroPagina){
    int marco = obtenerMarcoDesdePagina(nombreFile, tag, numeroPagina);
    
    
    if(marco != -1){
        log_debug(logger, "devolvio el marco: %d", marco);
        
        return marco;
    }
    else{
        log_info(logger,"Query <%d>: - Memoria Miss - File: <%s> - Tag: <%s> - Pagina: <%d>",contexto->query_id,nombreFile,tag,numeroPagina);
        char* contenido = traerPaginaDeStorage(nombreFile, tag, contexto->query_id, numeroPagina);
        if(!contenido){
            log_debug(logger, "Error al traer pagina de Storage %s:%s pagina %d", nombreFile, tag, numeroPagina);
            return -1;
        }
        char* key = string_from_format("%s:%s", nombreFile, tag); //Convertir nombreFile y tag en la key
        
        int marcoLibre = obtenerMarcoLibreYReservado(key, numeroPagina);
        if(marcoLibre == -1){
            log_error(logger, "Error al obtener marco libre para cargar pagina %s:%s pagina %d", nombreFile, tag, numeroPagina);
            free(contenido);
            exit(EXIT_FAILURE);
        }
        //esta funcion no va tener q reservar marco
        escribirEnMemoriaPaginaCompleta(nombreFile, tag, numeroPagina, marcoLibre, contenido, configW->BLOCK_SIZE);
        log_info(logger,"Query <%d>: - Memoria Add - File: <%s> - Tag: <%s> - Pagina: <%d> - Marco: <%d>",contexto->query_id,nombreFile,tag,numeroPagina,marcoLibre);

        free(key);
        free(contenido);

        aplicarRetardoMemoria();

        return marcoLibre;
    }
}

int obtenerMarcoDesdePagina(char* nombreFile, char* tag, int numeroPagina){
    log_debug(logger, "Obteniendo marco desde tabla de paginas para %s:%s pagina %d", nombreFile, tag, numeroPagina);
    TablaDePaginas* tabla =  obtenerTablaPorFileYTag(nombreFile, tag);

    if (!tabla) {
        log_debug(logger, "No existe tabla de paginas para %s:%s", nombreFile, tag);
        return -1;
    }

    if (numeroPagina < 0 || numeroPagina >= tabla->capacidadEntradas) {
        log_error(logger, "Numero de pagina %d invalido para %s:%s (max: %d)", 
                  numeroPagina, nombreFile, tag, tabla->capacidadEntradas - 1);
        return -1;
    }

    pthread_mutex_lock(&tabla_paginas_mutex);

    EntradaDeTabla *entrada = &tabla->entradas[numeroPagina];

    if(entrada->bitPresencia == false){
        log_debug(logger, "La pagina %d no está en memoria para el proceso %s:%s", numeroPagina, nombreFile, tag);
        pthread_mutex_unlock(&tabla_paginas_mutex);
        return -1;
    }
    int marco = entrada->numeroMarco;
    pthread_mutex_unlock(&tabla_paginas_mutex);

    aplicarRetardoMemoria(); 

    return marco;
}


char* traerPaginaDeStorage(char* nombreFile, char* tag, int query_id, int numeroPagina){
    log_debug(logger, "Solicitando a Storage la pagina %d de %s:%s", numeroPagina, nombreFile, tag);
    enviarOpcode(READ_BLOCK, socketStorage);


    t_paquete* paquete = crearPaquete();
    agregarIntAPaquete(paquete, query_id);
    agregarStringAPaquete(paquete, nombreFile);
    agregarStringAPaquete(paquete, tag);
    agregarIntAPaquete(paquete, numeroPagina);
    enviarPaquete(paquete, socketStorage);
    eliminarPaquete(paquete);

    log_debug(logger, "se solicito a Storage de %s:%s pagina %d", nombreFile, tag, numeroPagina);   

    char* contenido = escucharStorageContenidoPagina();

    return contenido;
}

int enviarPaginaAStorage(char* nombreFile, char* tag, int numeroPagina){
    char* contenido = obtenerContenidoDelMarco(obtenerNumeroDeMarco(nombreFile, tag, numeroPagina), 0, configW->BLOCK_SIZE); //esta bien usar obtener numero de marco aca?? Entendia q la usaba el query interpreter. a
    //si el contexto de ejecucion es global no habria problema
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
        log_debug(logger, "Error al enviar pagina a Storage %s:%s pagina %d", nombreFile, tag, numeroPagina);
        return -1;
    }

    return 1;
}


// Lectura en "Memoria Interna"
char* leerContenidoDesdeOffset(char* nombreFile, char* tag, int numeroPagina, int numeroMarco, int offset, int size){
    
    // Validar que el offset y size sean correctos, con el tamaño de la página
    // if(offset < 0 || size < 0 || offset + size > configW->BLOCK_SIZE){
    //     log_error(logger, "Error de lectura: offset %d + size %d excede el tamaño de la página %d", offset, size, configW->BLOCK_SIZE);
    //     return NULL;
    // }
    


    char* contenido = obtenerContenidoDelMarco(numeroMarco, offset, size);
    if(!contenido){
        return NULL;
    }

    TablaDePaginas* tabla = obtenerTablaPorFileYTag(nombreFile, tag);
    if(!tabla){
        log_error(logger, "Error al obtener tabla de paginas para %s:%s", nombreFile, tag);
        free(contenido);
        return NULL;
    }

    pthread_mutex_lock(&tabla_paginas_mutex);
    EntradaDeTabla* entrada = &tabla->entradas[numeroPagina];
    actualizar_acceso_pagina(entrada);

    pthread_mutex_unlock(&tabla_paginas_mutex);
    
    
    return contenido;
}

//Escritura en "Memoria Interna"
//revisar si es necesario pasarle el size
void escribirContenidoDesdeOffset(char* nombreFile, char* tag, int numeroPagina, int numeroMarco, char* contenido, int offset, int size){
    if(offset + size > configW->BLOCK_SIZE){
        log_error(logger, "Error de escritura: offset %d + size %d excede el tamanio de la pagina %d", offset, size, configW->BLOCK_SIZE);
        exit(EXIT_FAILURE);
        return;
    }
    
    TablaDePaginas* tabla = obtenerTablaPorFileYTag(nombreFile, tag);
    log_debug(logger, "1. Escribiendo en tabla de paginas para %s:%s pagina %d", nombreFile, tag, numeroPagina);
    if(!tabla){
        log_error(logger, "Error al obtener tabla de paginas para %s:%s", nombreFile, tag);
        exit(EXIT_FAILURE);
    }
    
    if(strcmp(tabla->keyProceso, (string_from_format("%s:%s", nombreFile, tag))) != 0){
        log_error(logger, "El Marco %d NO pertenece al FILE:TAG %s:%s", numeroMarco, nombreFile, tag);
        exit(EXIT_FAILURE);
    }

    escribirMarcoConOffset(numeroMarco, contenido, offset,size);

    pthread_mutex_lock(&tabla_paginas_mutex);

    tabla->hayPaginasModificadas = true;
    EntradaDeTabla* entrada = &tabla->entradas[numeroPagina];

    modificar_pagina(entrada);

    pthread_mutex_unlock(&tabla_paginas_mutex);

    log_debug(logger, "Antes de aplicar retardo de memoria en escribirContenidoDesdeOffset");   

    aplicarRetardoMemoria();
    return;
}

void escribirMarcoConOffset(int numeroMarco, char* contenido, int offset, int size){
    pthread_mutex_lock(&memoria_mutex);
    memcpy(memoria + (numeroMarco * configW->BLOCK_SIZE) + offset, contenido, size);
    pthread_mutex_unlock(&memoria_mutex);
    return;
}


//tiene que revisar si el file:tag ya tiene la pagina en la tabla de paginas. o revisar si la pagina esta en la tabla de paginas (aunque sea ausente)
void escribirEnMemoriaPaginaCompleta(char* nombreFile, char* tag, int numeroPagina, int marcoLibre, char* contenidoPagina, int size){ // Solo se usa cuando se trae la pagina de Storage
    // Validación de tamaño
    if (size > configW->BLOCK_SIZE) {
        log_error(logger, "Error: size %d excede BLOCK_SIZE %d", size, configW->BLOCK_SIZE);
        exit(EXIT_FAILURE);
    }
    
    // Validez del marco
    if(marcoLibre < 0 || marcoLibre >= cant_marcos){
        log_error(logger, "Numero de marco %d invalido (max: %d)", marcoLibre, cant_marcos - 1);
        exit(EXIT_FAILURE);
    }
    log_debug(logger, "Escribiendo pagina completa en memoria para %s:%s pagina %d en marco %d", nombreFile, tag, numeroPagina, marcoLibre);
    asignarMarcoEntradaTabla(nombreFile, tag, numeroPagina, marcoLibre);


    TablaDePaginas* tabla = obtenerTablaPorFileYTag(nombreFile, tag);
    if(!tabla){
        log_error(logger, "Error al obtener tabla de paginas para %s:%s", nombreFile, tag);
        exit(EXIT_FAILURE);
    }
    pthread_mutex_lock(&tabla_paginas_mutex);
    EntradaDeTabla* entrada = &tabla->entradas[numeroPagina];

    entrada->bitModificado = false;

    tabla->hayPaginasModificadas = true;
    pthread_mutex_unlock(&tabla_paginas_mutex);

    log_info(logger,"Query <%d>: Se asigna el Marco: <%d> a la Página: <%d> perteneciente al - File: <%s> - Tag: <%s>",contexto->query_id,marcoLibre,numeroPagina,nombreFile,tag);
    
    // Escribir en memoria fisica
    escribirMarcoConOffset(marcoLibre, contenidoPagina,0,size);

    aplicarRetardoMemoria();
    
    actualizarMetadataTablaPagina(tabla);
    
    return;
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


//Devuelve un FileTag a reemplazar
key_Reemplazo* ejecutarAlgoritmoReemplazo() {
    if (string_equals_ignore_case(configW->algoritmoReemplazo, "LRU")) {
        return ReemplazoLRU(); 
    } 
    else if (string_equals_ignore_case(configW->algoritmoReemplazo, "CLOCK-M")) {
        return ReemplazoCLOCKM();
    } 
    else {
        log_error(logger, "Algoritmo de reemplazo desconocido: %s", configW->algoritmoReemplazo);
    }
    
    return NULL;
}


int64_t obtener_tiempo_actual() {
    return temporal_gettime(temp);
}

void inicializar_entrada(EntradaDeTabla* entrada, int numeroPagina) {
    entrada->numeroPagina = numeroPagina;
    entrada->numeroMarco = -1;  // Sin marco asignado aún
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
    log_debug(logger, "tiempo actual actualizado en modificar_pagina: %ld", entrada->ultimoAcceso);
    entrada->bitUso = true;
    entrada->bitModificado = true;
}

