#include "algoritmosReemplazo.h"


static PunteroClockModificado punteroClockMod = {NULL, 0};

key_Reemplazo* ReemplazoLRU() {
    key_Reemplazo* keyReemplazo = malloc(sizeof(key_Reemplazo));  
    keyReemplazo->key = NULL;
    keyReemplazo->marco = -1;
    keyReemplazo->pagina = -1;

    int64_t menorTiempo = INT64_MAX;

    log_debug(logger,"Estamos dentro del LRU");
    pthread_mutex_lock(&tabla_paginas_mutex);
    t_list* keys = dictionary_keys(tablasDePaginas);
    log_debug(logger,"Log +1");

    for(int i = 0; i < list_size(keys); i++) {
        char* currentKey = list_get(keys, i);
        TablaDePaginas* tabla = dictionary_get(tablasDePaginas, currentKey);

        for(int j = 0; j < tabla->capacidadEntradas; j++) {
            EntradaDeTabla* entrada = &(tabla->entradas[j]);

            if(entrada->bitPresencia) {
                log_warning(logger,"Revisando %s página %d: último acceso %ld, acceso a comparar: %ld", currentKey, entrada->numeroPagina, entrada->ultimoAcceso, menorTiempo);
                if(entrada->ultimoAcceso < menorTiempo) {
                    menorTiempo = entrada->ultimoAcceso;
                    
                    if(keyReemplazo->key != NULL) {
                        free(keyReemplazo->key);
                    }

                    keyReemplazo->key = strdup(currentKey);
                    keyReemplazo->marco = entrada->numeroMarco;
                    keyReemplazo->pagina = entrada->numeroPagina;
                }
            }
        }
    }

    pthread_mutex_unlock(&tabla_paginas_mutex);
    list_destroy(keys);

    return keyReemplazo; 
}


//se debe hacer el free d
key_Reemplazo* ReemplazoCLOCKM() {
    key_Reemplazo* keyReemplazo = (key_Reemplazo*)malloc(sizeof(key_Reemplazo));
    EntradaDeTabla* entradaVictima = NULL;
    bool encontrado = false;
    
    pthread_mutex_lock(&tabla_paginas_mutex);

    t_list* keys = dictionary_keys(tablasDePaginas);
    int totalProcesos = list_size(keys);
    
    if(totalProcesos == 0) {
        list_destroy(keys);
        pthread_mutex_unlock(&tabla_paginas_mutex);
        free(keyReemplazo);
        return NULL;
    }
    
    // Inicializar puntero si es la primera vez
    if(punteroClockMod.keyProceso == NULL) {
        punteroClockMod.keyProceso = strdup(list_get(keys, 0));
        punteroClockMod.indicePagina = 0;
        // log_error(logger, "El puntero clock no esta inicializado");

    }

    char* key = NULL;
    int intentos = 0;
    const int MAX_INTENTOS = 10; // Límite de seguridad para evitar bucle infinito

    
    while(!encontrado && intentos < MAX_INTENTOS) {
        intentos++;
        
        // PASO 1: Buscar (0,0) SIN limpiar bits de uso
        log_debug(logger, "Clock-M intento %d: Paso 1 - Buscando (0,0) sin limpiar", intentos);
        encontrado = buscar_victima_clock(keys, &entradaVictima, &key, false, false, false);
        
        if(!encontrado) {
            // PASO 2: Buscar (0,1) LIMPIANDO bits de uso
            log_debug(logger, "Clock-M intento %d: Paso 2 - Buscando (0,1) limpiando bits", intentos);
            encontrado = buscar_victima_clock(keys, &entradaVictima, &key, false, true, true);
        }
        
        // Si no encontró, vuelve al paso 1 automáticamente (siguiente iteración del while)
    }
    
    if(!encontrado) {
        log_error(logger, "Clock-M: No se encontró víctima después de %d intentos", intentos);
        list_destroy(keys);
        exit(EXIT_FAILURE);
        pthread_mutex_unlock(&tabla_paginas_mutex);
        return NULL;
    }

    keyReemplazo->key = strdup(key);  // ← CAMBIO CRÍTICO
    keyReemplazo->marco = entradaVictima->numeroMarco;
    keyReemplazo->pagina = entradaVictima->numeroPagina;
    
    list_destroy(keys);
    pthread_mutex_unlock(&tabla_paginas_mutex);
    
    log_debug(logger, "Clock-M: Víctima encontrada en intento %d - %s, Página: %d, Frame: %d", 
              intentos, keyReemplazo->key, entradaVictima->numeroPagina, entradaVictima->numeroMarco);
    
    return keyReemplazo;
}

key_Reemplazo* ReemplazoCLOCKMTrucho() {
    key_Reemplazo* keyReemplazo = (key_Reemplazo*)malloc(sizeof(key_Reemplazo));
    EntradaDeTabla* entradaVictima = NULL;
    bool encontrado = false;
    
    pthread_mutex_lock(&tabla_paginas_mutex);

    t_list* keys = dictionary_keys(tablasDePaginas);
    int totalProcesos = list_size(keys);
    
    if(totalProcesos == 0) {
        list_destroy(keys);
        pthread_mutex_unlock(&tabla_paginas_mutex);
        free(keyReemplazo);
        return NULL;
    }
    
    // Inicializar puntero si es la primera vez
    if(punteroClockMod.keyProceso == NULL) {
        punteroClockMod.keyProceso = strdup(list_get(keys, 0));
        punteroClockMod.indicePagina = 0;
    }

    char* key = NULL;
    
    // Ciclo principal: repetir hasta encontrar víctima
    while(!encontrado) {
        // PASO 1: Buscar (0,0) SIN limpiar bits de uso
        log_debug(logger, "Clock-M: Paso 1 - Buscando (U=0, M=0) sin limpiar bits");
        encontrado = buscar_victima_clock(keys, &entradaVictima, &key, false, false, false);
        
        if(!encontrado) {
            // PASO 2: Buscar (0,1) LIMPIANDO bits de uso
            log_debug(logger, "Clock-M: Paso 2 - Buscando (U=0, M=1) limpiando bits de uso");
            encontrado = buscar_victima_clock(keys, &entradaVictima, &key, false, true, true);
        }
        
        // Si el paso 2 limpió bits pero no encontró (0,1), 
        // el while vuelve al paso 1 donde ahora SÍ va a encontrar (0,0)
        
        if(!encontrado) {
            log_error(logger, "Clock-M: No se encontró víctima en ciclo completo - esto no debería pasar");
            list_destroy(keys);
            free(keyReemplazo);
            pthread_mutex_unlock(&tabla_paginas_mutex);
            exit(EXIT_FAILURE);
        }
    }

    keyReemplazo->key = strdup(key);
    keyReemplazo->marco = entradaVictima->numeroMarco;
    keyReemplazo->pagina = entradaVictima->numeroPagina;
    
    list_destroy(keys);
    pthread_mutex_unlock(&tabla_paginas_mutex);
    
    log_info(logger, "Clock-M: Víctima encontrada - %s, Página: %d, Frame: %d", 
              keyReemplazo->key, entradaVictima->numeroPagina, entradaVictima->numeroMarco);
    
    return keyReemplazo;
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
    int fileTagActual = encontrar_indice_fileTag(keys, punteroClockMod.keyProceso);
    int paginaActual = punteroClockMod.indicePagina;
    
    int cantidadMarcos = configW->tamMemoria / configW->BLOCK_SIZE;
    if(cantidadMarcos <= 0) {
        log_error(logger, "Error: cantidad de marcos es %d", cantidadMarcos);
        return false;
    }
    
    // Guardar posición inicial para detectar vuelta completa
    int fileTagInicial = fileTagActual;
    int paginaInicial = paginaActual;
    bool primeraIteracion = true;
    
    int paginasRevisadas = 0;
    
    // Dar una vuelta completa al reloj
    while(paginasRevisadas < cantidadMarcos) {
        char* currentKey = list_get(keys, fileTagActual);
        TablaDePaginas* tabla = dictionary_get(tablasDePaginas, currentKey);
        
        if(!tabla) {
            log_error(logger, "Tabla NULL para key: %s", currentKey);
            return false;
        }
        
        // Recorrer las páginas de esta tabla desde paginaActual
        for(int j = paginaActual; j < tabla->capacidadEntradas; j++) {
            EntradaDeTabla* entrada = &(tabla->entradas[j]);
            
            if(entrada->bitPresencia) {
                paginasRevisadas++;
                
                // Verificar si cumple el criterio buscado
                if(entrada->bitUso == buscarBitUso && entrada->bitModificado == buscarBitMod) {
                    // ¡Encontramos la víctima!
                    *victima = entrada;
                    *keyOut = currentKey;
                    
                    // Avanzar el puntero del reloj para la próxima búsqueda
                    int proximaPagina = j + 1;
                    int proximoFileTag = fileTagActual;
                    
                    if(proximaPagina >= tabla->capacidadEntradas) {
                        proximoFileTag = (fileTagActual + 1) % totalProcesos;
                        proximaPagina = 0;
                    }
                    
                    if(punteroClockMod.keyProceso != NULL) {
                        free(punteroClockMod.keyProceso);
                    }
                    punteroClockMod.keyProceso = strdup(list_get(keys, proximoFileTag));
                    punteroClockMod.indicePagina = proximaPagina;
                    
                    log_debug(logger, "Víctima: %s pág %d frame %d (U=%d,M=%d). Puntero → %s:%d", 
                              currentKey, entrada->numeroPagina, entrada->numeroMarco,
                              entrada->bitUso, entrada->bitModificado,
                              punteroClockMod.keyProceso, punteroClockMod.indicePagina);
                    
                    return true;
                }
                
                // Limpiar bit de uso si corresponde (paso 2)
                if(limpiarBitUso) {
                    entrada->bitUso = false;
                }
                
                // Verificar si completamos la vuelta
                if(!primeraIteracion && 
                   fileTagActual == fileTagInicial && 
                   j >= paginaInicial) {
                    log_debug(logger, "Vuelta completa - No encontrado (U=%d,M=%d)", 
                              buscarBitUso, buscarBitMod);
                    return false;
                }
            }
        }
        
        primeraIteracion = false;
        
        // Avanzar al siguiente file:tag
        fileTagActual = (fileTagActual + 1) % totalProcesos;
        paginaActual = 0;
        
        // Si volvimos al file:tag inicial, ajustar paginaActual
        if(fileTagActual == fileTagInicial) {
            paginaActual = paginaInicial;
        }
    }
    
    return false;
}

bool buscar_victima_clockTrucho(t_list* keys, EntradaDeTabla** victima, char** keyOut, 
                          bool buscarBitUso, bool buscarBitMod, bool limpiarBitUso) {
    int totalProcesos = list_size(keys);
    if(totalProcesos == 0) return false;
    
    int fileTagActual = encontrar_indice_fileTag(keys, punteroClockMod.keyProceso);
    int paginaActual = punteroClockMod.indicePagina;
    
    // Contar marcos TOTALES del sistema (no solo presentes)
    int cantidadMarcos = configW->tamMemoria / configW->BLOCK_SIZE;
    if(cantidadMarcos <= 0) {
        log_error(logger, "Error: cantidad de marcos es %d", cantidadMarcos);
        return false;
    }
    
    int fileTagInicial = fileTagActual;
    int paginaInicial = paginaActual;
    int paginasRevisadas = 0;
    bool primeraVez = true;
    
    log_debug(logger, "Clock-M búsqueda: iniciando desde %s:%d (U=%d, M=%d, limpiar=%d)", 
              list_get(keys, fileTagActual), paginaActual, 
              buscarBitUso, buscarBitMod, limpiarBitUso);
    
    // Dar UNA vuelta completa al reloj
    while(true) {
        char* currentKey = list_get(keys, fileTagActual);
        TablaDePaginas* tabla = dictionary_get(tablasDePaginas, currentKey);
        
        if(!tabla) {
            log_error(logger, "Tabla NULL para key: %s", currentKey);
            return false;
        }
        
        // Recorrer páginas de esta tabla
        for(int j = paginaActual; j < tabla->capacidadEntradas; j++) {
            EntradaDeTabla* entrada = &(tabla->entradas[j]);
            
            // Solo considerar páginas PRESENTES en memoria
            if(entrada->bitPresencia) {
                paginasRevisadas++;
                
                log_debug(logger, "  Revisando %s[%d]: U=%d M=%d (buscando U=%d M=%d)", 
                          currentKey, j, entrada->bitUso, entrada->bitModificado,
                          buscarBitUso, buscarBitMod);
                
                // Verificar si cumple el criterio buscado
                if(entrada->bitUso == buscarBitUso && entrada->bitModificado == buscarBitMod) {
                    // ¡Encontramos la víctima!
                    *victima = entrada;
                    *keyOut = currentKey;
                    
                    log_info(logger, "Clock-M: Víctima seleccionada %s[%d] marco %d (U=%d, M=%d)", 
                             currentKey, j, entrada->numeroMarco, 
                             entrada->bitUso, entrada->bitModificado);
                    
                    // Avanzar el puntero del reloj para la próxima búsqueda
                    int proximaPagina = j + 1;
                    int proximoFileTag = fileTagActual;
                    
                    if(proximaPagina >= tabla->capacidadEntradas) {
                        proximoFileTag = (fileTagActual + 1) % totalProcesos;
                        proximaPagina = 0;
                    }
                    
                    if(punteroClockMod.keyProceso != NULL) {
                        free(punteroClockMod.keyProceso);
                    }
                    punteroClockMod.keyProceso = strdup(list_get(keys, proximoFileTag));
                    punteroClockMod.indicePagina = proximaPagina;
                    
                    log_debug(logger, "  Puntero Clock avanza a %s:%d", 
                              punteroClockMod.keyProceso, punteroClockMod.indicePagina);
                    
                    return true;
                }
                
                // Limpiar bit de uso si corresponde (paso 2 del algoritmo)
                if(limpiarBitUso && entrada->bitUso) {
                    log_debug(logger, "  Limpiando bit U de %s[%d]", currentKey, j);
                    entrada->bitUso = false;
                }
                
                // Si ya revisamos todos los marcos, terminamos la vuelta
                if(paginasRevisadas >= cantidadMarcos) {
                    log_debug(logger, "Clock-M: Vuelta completa (%d marcos revisados), no encontrado (U=%d, M=%d)", 
                              paginasRevisadas, buscarBitUso, buscarBitMod);
                    return false;
                }
            }
            
            // Detectar vuelta completa (volvimos al punto inicial)
            if(!primeraVez && fileTagActual == fileTagInicial && j == paginaInicial) {
                log_debug(logger, "Clock-M: Vuelta completa detectada por posición inicial");
                return false;
            }
        }
        
        primeraVez = false;
        paginaActual = 0; // Al cambiar de file:tag, empezar desde página 0
        
        // Avanzar al siguiente file:tag
        fileTagActual = (fileTagActual + 1) % totalProcesos;
        
        // Si volvimos al file:tag inicial, ajustar la página de inicio
        if(fileTagActual == fileTagInicial) {
            paginaActual = paginaInicial;
        }
    }
    
    return false;
}

int encontrar_indice_fileTag(t_list* keys, char* keyBuscada) {
    for(int i = 0; i < list_size(keys); i++) {
        if(strcmp(list_get(keys, i), keyBuscada) == 0) {
            return i;
        }
    }
    return 0;
}

//Cuenta la cantidad de paginas presentes en memoria de todas las tablas //marcos ocupados
// int contar_paginas_presentes(t_list* keys) {
//     int count = 0;
//     for(int i = 0; i < list_size(keys); i++) {
//         char* currentKey = list_get(keys, i);
//         TablaDePaginas* tabla = dictionary_get(tablasDePaginas, currentKey);
//         count += tabla->paginasPresentes;
//     }
//     return count;
// }