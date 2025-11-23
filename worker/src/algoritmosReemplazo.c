#include "algoritmosReemplazo.h"

static PunteroClockModificado punteroClockMod = {NULL, 0};


key_Reemplazo* ReemplazoLRU() {
    key_Reemplazo* keyReemplazo = malloc(sizeof(key_Reemplazo));  
    keyReemplazo->key = NULL;
    keyReemplazo->marco = -1;
    keyReemplazo->pagina = -1;

    int64_t menorTiempo = INT64_MAX;

    pthread_mutex_lock(&tabla_paginas_mutex);

    for(int marcoIdx = 0; marcoIdx < cant_marcos; marcoIdx++) {
        // Obtener la página que está en este marco (si hay alguna)
        PaginaXMarco* pxm = vector_pxm_get(vectorPaginaXMarco, marcoIdx);
        
        if(pxm != NULL && pxm->key != NULL) {
            // Este marco está ocupado, verificar su timestamp
            char* nombreFile = NULL;
            char* tag = NULL;
            
            if(!ObtenerNombreFileYTagInterna(pxm->key, &nombreFile, &tag)) {
                log_error(logger, "LRU: Error parseando key %s en marco %d", pxm->key, marcoIdx);
                free(nombreFile);
                free(tag);
                continue;
            }
            
            TablaDePaginas* tabla = dictionary_get(tablasDePaginas, pxm->key);
            if(!tabla) {
                log_error(logger, "LRU: Tabla NULL para key %s en marco %d", pxm->key, marcoIdx);
                free(nombreFile);
                free(tag);
                continue;
            }
            
            // Verificar que el número de página sea válido
            if(pxm->nroPagina < 0 || pxm->nroPagina >= tabla->capacidadEntradas) {
                log_error(logger, "LRU: Número de página inválido %d en marco %d", 
                          pxm->nroPagina, marcoIdx);
                free(nombreFile);
                free(tag);
                continue;
            }
            
            EntradaDeTabla* entrada = &(tabla->entradas[pxm->nroPagina]);
            
            // Verificar consistencia: debe estar presente y el marco debe coincidir
            if(!entrada->bitPresencia || entrada->numeroMarco != marcoIdx) {
                log_warning(logger, "LRU: Inconsistencia en marco %d: esperaba pág %d de %s, "
                           "pero bitPresencia=%d o marco=%d", 
                           marcoIdx, pxm->nroPagina, pxm->key, 
                           entrada->bitPresencia, entrada->numeroMarco);
                free(nombreFile);
                free(tag);
                continue;
            }
            
            // Comparar timestamp
            log_warning(logger, "LRU: Revisando marco %d - %s[pág %d]: último acceso %ld vs menorTiempo %ld", 
                      marcoIdx, pxm->key, pxm->nroPagina, entrada->ultimoAcceso, menorTiempo);
            
            if(entrada->ultimoAcceso < menorTiempo) {
                menorTiempo = entrada->ultimoAcceso;
                
                if(keyReemplazo->key != NULL) {
                    free(keyReemplazo->key);
                }
                
                keyReemplazo->key = strdup(pxm->key);
                keyReemplazo->marco = entrada->numeroMarco;
                keyReemplazo->pagina = entrada->numeroPagina;
                
                log_debug(logger, "LRU: Nueva víctima candidata - marco %d, %s[pág %d], tiempo %ld", 
                          marcoIdx, pxm->key, pxm->nroPagina, entrada->ultimoAcceso);
            }
            
            free(nombreFile);
            free(tag);
        }
    }

    pthread_mutex_unlock(&tabla_paginas_mutex);

    if(keyReemplazo->key == NULL) {
        log_error(logger, "LRU: No se encontró ninguna página para reemplazar");
        free(keyReemplazo);
        return NULL;
    }

    log_info(logger, "LRU: Víctima seleccionada - marco %d: %s[pág %d] con tiempo %ld", 
             keyReemplazo->marco, keyReemplazo->key, keyReemplazo->pagina, menorTiempo);

    return keyReemplazo; 
}


//se debe hacer el free d
key_Reemplazo* ReemplazoCLOCKM() {
    key_Reemplazo* keyReemplazo = (key_Reemplazo*)malloc(sizeof(key_Reemplazo));
    EntradaDeTabla* entradaVictima = NULL;
    bool encontrado = false;
    
    pthread_mutex_lock(&tabla_paginas_mutex);
    
    if(vectorPaginaXMarco->cantidad == 0) {
        pthread_mutex_unlock(&tabla_paginas_mutex);
        free(keyReemplazo);
        log_error(logger, "Clock-M: No hay marcos ocupados en memoria");
        return NULL;
    }
    
    // Inicializar puntero si es la primera vez
    if(punteroClockMod.indiceMarco < 0 || punteroClockMod.indiceMarco >= cant_marcos) {
        punteroClockMod.indiceMarco = 0;
        if(punteroClockMod.keyProceso != NULL) {
            free(punteroClockMod.keyProceso);
            punteroClockMod.keyProceso = NULL;
        }
    }

    char* key = NULL;
    int intentos = 0;
    const int MAX_INTENTOS = 10; // Límite de seguridad

    
    while(!encontrado && intentos < MAX_INTENTOS) {
        intentos++;
        
        // PASO 1: Buscar (0,0) SIN limpiar bits de uso
        log_debug(logger, "Clock-M intento %d: Paso 1 - Buscando (U=0, M=0) sin limpiar", intentos);
        encontrado = buscar_victima_clock(&entradaVictima, &key, false, false, false);
        
        if(!encontrado) {
            // PASO 2: Buscar (0,1) LIMPIANDO bits de uso
            log_debug(logger, "Clock-M intento %d: Paso 2 - Buscando (U=0, M=1) limpiando bits", intentos);
            encontrado = buscar_victima_clock(&entradaVictima, &key, false, true, true);
        }
    }
    if(!encontrado) {
        log_error(logger, "Clock-M: No se encontró víctima después de %d intentos", intentos);
        pthread_mutex_unlock(&tabla_paginas_mutex);
        free(keyReemplazo);
        exit(EXIT_FAILURE);
    }

    keyReemplazo->key = strdup(key);
    keyReemplazo->marco = entradaVictima->numeroMarco;
    keyReemplazo->pagina = entradaVictima->numeroPagina;
    
    pthread_mutex_unlock(&tabla_paginas_mutex);
    
    log_debug(logger, "Clock-M: Víctima encontrada en intento %d - %s, Página: %d, Frame: %d", 
              intentos, keyReemplazo->key, entradaVictima->numeroPagina, entradaVictima->numeroMarco);
    
    return keyReemplazo;
}

bool buscar_victima_clock(EntradaDeTabla** victima, char** keyOut, 
                                       bool buscarBitUso, bool buscarBitMod, bool limpiarBitUso) {
    
    int indiceMarcoActual = punteroClockMod.indiceMarco;
    int indiceMarcoInicial = indiceMarcoActual;
    int marcosRevisados = 0;
    bool primeraIteracion = true;
    
    log_debug(logger, "Clock-M búsqueda vectorizada: iniciando desde marco %d (U=%d, M=%d, limpiar=%d)", 
              indiceMarcoActual, buscarBitUso, buscarBitMod, limpiarBitUso);
    
    // Dar una vuelta completa al reloj
    while(marcosRevisados < cant_marcos) {
        // Obtener la página en este marco usando el vector indexado por marco
        PaginaXMarco* pxm = vector_pxm_get(vectorPaginaXMarco, indiceMarcoActual);
        
        if(pxm != NULL && pxm->key != NULL) {
            // Este marco está ocupado, revisar su entrada de tabla
            char* nombreFile = NULL;
            char* tag = NULL;
            
            if(!ObtenerNombreFileYTagInterna(pxm->key, &nombreFile, &tag)) {
                log_error(logger, "Clock-M: Error parseando key %s en marco %d", pxm->key, indiceMarcoActual);
                indiceMarcoActual = (indiceMarcoActual + 1) % cant_marcos;
                marcosRevisados++;
                continue;
            }
            
            TablaDePaginas* tabla = dictionary_get(tablasDePaginas, pxm->key);
            if(!tabla) {
                log_error(logger, "Clock-M: Tabla NULL para key %s en marco %d", pxm->key, indiceMarcoActual);
                free(nombreFile);
                free(tag);
                indiceMarcoActual = (indiceMarcoActual + 1) % cant_marcos;
                marcosRevisados++;
                continue;
            }
            
            // Verificar que el número de página sea válido
            if(pxm->nroPagina < 0 || pxm->nroPagina >= tabla->capacidadEntradas) {
                log_error(logger, "Clock-M: Número de página inválido %d en marco %d", 
                          pxm->nroPagina, indiceMarcoActual);
                free(nombreFile);
                free(tag);
                indiceMarcoActual = (indiceMarcoActual + 1) % cant_marcos;
                marcosRevisados++;
                continue;
            }
            
            EntradaDeTabla* entrada = &(tabla->entradas[pxm->nroPagina]);
            
            // Verificar que la entrada realmente esté presente y corresponda al marco
            if(!entrada->bitPresencia || entrada->numeroMarco != indiceMarcoActual) {
                log_warning(logger, "Clock-M: Inconsistencia en marco %d: esperaba pág %d de %s, "
                           "pero bitPresencia=%d o marco=%d", 
                           indiceMarcoActual, pxm->nroPagina, pxm->key, 
                           entrada->bitPresencia, entrada->numeroMarco);
                free(nombreFile);
                free(tag);
                indiceMarcoActual = (indiceMarcoActual + 1) % cant_marcos;
                marcosRevisados++;
                continue;
            }
            
            log_warning(logger, "  Revisando marco %d: %s[pág %d] U=%d M=%d (buscando U=%d M=%d)", 
                      indiceMarcoActual, pxm->key, pxm->nroPagina,
                      entrada->bitUso, entrada->bitModificado,
                      buscarBitUso, buscarBitMod);
            
            // Verificar si cumple el criterio buscado
            if(entrada->bitUso == buscarBitUso && entrada->bitModificado == buscarBitMod) {
                // ¡Encontramos la víctima!
                *victima = entrada;
                *keyOut = pxm->key;
                
                log_info(logger, "Clock-M: Víctima seleccionada marco %d: %s[pág %d] (U=%d, M=%d)", 
                         indiceMarcoActual, pxm->key, pxm->nroPagina,
                         entrada->bitUso, entrada->bitModificado);
                
                // Avanzar el puntero del reloj para la próxima búsqueda
                punteroClockMod.indiceMarco = (indiceMarcoActual + 1) % cant_marcos;
                
                if(punteroClockMod.keyProceso != NULL) {
                    free(punteroClockMod.keyProceso);
                }
                punteroClockMod.keyProceso = strdup(pxm->key);
                
                log_debug(logger, "  Puntero Clock avanza a marco %d (%s)", 
                          punteroClockMod.indiceMarco, punteroClockMod.keyProceso);
                
                free(nombreFile);
                free(tag);
                return true;
            }
            
            // Limpiar bit de uso si corresponde (paso 2 del algoritmo)
            if(limpiarBitUso && entrada->bitUso) {
                log_debug(logger, "  Limpiando bit U de marco %d: %s[pág %d]", 
                          indiceMarcoActual, pxm->key, pxm->nroPagina);
                entrada->bitUso = false;
            }
            
            free(nombreFile);
            free(tag);
        }
        
        // Avanzar al siguiente marco
        indiceMarcoActual = (indiceMarcoActual + 1) % cant_marcos;
        marcosRevisados++;
        
        // Detectar vuelta completa
        if(!primeraIteracion && indiceMarcoActual == indiceMarcoInicial) {
            log_debug(logger, "Clock-M: Vuelta completa (%d marcos revisados), no encontrado (U=%d, M=%d)", 
                      marcosRevisados, buscarBitUso, buscarBitMod);
            return false;
        }
        
        primeraIteracion = false;
    }
    
    log_debug(logger, "Clock-M: Se revisaron todos los %d marcos, no encontrado (U=%d, M=%d)", 
              marcosRevisados, buscarBitUso, buscarBitMod);
    return false;
}

void limpiar_puntero_clockM() {
    if(punteroClockMod.keyProceso != NULL) {
        free(punteroClockMod.keyProceso);
        punteroClockMod.keyProceso = NULL;
    }
    punteroClockMod.indiceMarco = 0; // Resetear a marco 0
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


char* ObtenerNombreFileYTagInterna(const char* fileTagText, char** fileOut, char** tagOut) {
    if (!fileTagText || !fileOut || !tagOut) return NULL;

    char** splitParts = string_split((char*)fileTagText, ":");
    if (!splitParts) return NULL;

    bool formatoValido =
        splitParts[0] && splitParts[1] &&
        string_array_size(splitParts) == 2 &&
        !string_is_empty(splitParts[0]) &&
        !string_is_empty(splitParts[1]);

    if (!formatoValido) {
        string_array_destroy(splitParts);
        return NULL;
    }

    char* fileCopy = string_duplicate(splitParts[0]);
    char* tagCopy  = string_duplicate(splitParts[1]);
    string_array_destroy(splitParts);

    if (!fileCopy || !tagCopy) {
        free(fileCopy);
        free(tagCopy);
        return NULL;
    }

    *fileOut = fileCopy;
    *tagOut  = tagCopy;

    // Éxito: devolvemos un puntero no-NULL sin implicar ownership
    return (char*)fileTagText;
}