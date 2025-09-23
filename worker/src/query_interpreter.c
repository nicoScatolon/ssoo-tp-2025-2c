#include "query_interpreter.h"




contexto_query_t* cargarQuery(char* path, int query_id, int pc_inicial) { 
    #define BUF_SZ 512
    //la primera vez que se llama es con pc_inicial = 0
    if (path == NULL) {
        log_error(logger, "Ruta de query NULL");
        return NULL;
    } 

    size_t base_len = strlen(configW->pathQueries);
    size_t path_len = strlen(path);
    int need_slash = (base_len > 0 && configW->pathQueries[base_len - 1] != '/');
    size_t len = base_len + path_len + (need_slash ? 1 : 0) + 1; // need_slash == 0 o 1 + '\0' 

    char *path_completo = malloc(len);
    if (!path_completo){
        log_error(logger, "Error de memoria al construir path completo de query");
        return NULL;
    }
    if (need_slash)
        snprintf(path_completo, len, "%s/%s", configW->pathQueries, path);
    else
        snprintf(path_completo, len, "%s%s", configW->pathQueries, path);


    
    FILE* archivo = fopen(path_completo, "r");
    if (archivo == NULL) {
        log_error(logger, "No se pudo abrir el archivo de query: %s", path_completo);
        free(path_completo);
        return NULL;
    }
    
    contexto_query_t* contexto = malloc(sizeof(contexto_query_t));
    contexto->path_query = strdup(path);
    contexto->query_id = query_id;
    contexto->pc = pc_inicial;
    contexto->total_lineas = 0;
    
    char buffer[BUF_SZ];
    contexto->total_lineas = 0;

    /* primera pasada: contar líneas */
    while (fgets(buffer, sizeof(buffer), archivo) != NULL) {
        contexto->total_lineas++;
    }

    /* reservar sólo si hay líneas */
    if (contexto->total_lineas == 0) {
        contexto->lineas_query = NULL;
    } else {
        contexto->lineas_query = malloc(contexto->total_lineas * sizeof(char*));
    }

    /* volver al inicio y llenar el arreglo */
    rewind(archivo);
    int i = 0;
    while (fgets(buffer, sizeof(buffer), archivo) != NULL && i < contexto->total_lineas) {
        buffer[strcspn(buffer, "\r\n")] = '\0';   /* elimina \r y \n */
        contexto->lineas_query[i] = strdup(buffer);
        i++;
    }
    
    fclose(archivo);
    free(path_completo);
    log_debug(logger, "Query cargada: %s con %d lineas", path, contexto->total_lineas);
    
    return contexto;
}


tipo_instruccion_t obtenerTipoInstruccion(char* nombre) {
    if (strcmp(nombre, "CREATE") == 0) return CREATE;
    if (strcmp(nombre, "TRUNCATE") == 0) return TRUNCATE;
    if (strcmp(nombre, "WRITE") == 0) return WRITE;
    if (strcmp(nombre, "READ") == 0) return READ;
    if (strcmp(nombre, "TAG") == 0) return TAG;
    if (strcmp(nombre, "COMMIT") == 0) return COMMIT;
    if (strcmp(nombre, "FLUSH") == 0) return FLUSH;
    if (strcmp(nombre, "DELETE") == 0) return DELETE;
    if (strcmp(nombre, "END") == 0) return END;
    
    return DESCONOCIDO;
}

instruccion_t* parsearInstruccion(char* linea) {
    if (linea == NULL || strlen(linea) == 0) {
        return NULL;
    }
    
    instruccion_t* instruccion = malloc(sizeof(instruccion_t));
    if (!instruccion) return NULL;

    for (int i = 0; i < MAX_PARAMETROS; ++i)
        instruccion->parametro[i] = NULL;
    instruccion->tipo = DESCONOCIDO;

    
    char* linea_copia = strdup(linea);
    if (!linea_copia) {
        free(instruccion); 
        return NULL; 
    }
    
    char* token = strtok(linea_copia, " ");
    int idx = 0;
    while (token != NULL && idx < MAX_PARAMETROS) {
        instruccion->parametro[idx] = strdup(token);
        token = strtok(NULL, " ");
        idx++;
    }

    if (instruccion->parametro[0] != NULL) {
        instruccion->tipo = obtenerTipoInstruccion(instruccion->parametro[0]);
    }    
    
    
    free(linea_copia);
    return instruccion;
}


//estan por las dudas, no usar
char* obtenerNombreFile(const char* file_Y_tag) {
    const char* separador = strchr(file_Y_tag, ':');
    if (separador != NULL) {
        size_t len_nombre = separador - file_Y_tag;
        return strndup(file_Y_tag, len_nombre);
    } else {
        return strdup(file_Y_tag);
    }
}

char* obtenerNombreTag(const char* file_Y_tag) {
    if (!file_Y_tag){
        return NULL;  // opcional: manejar NULL
        log_error(logger, "el file_Y_tag no corresponde al formato correcto FILE:TAG: %s", file_Y_tag);
    }
    
    const char* sep = strchr(file_Y_tag, ':');
    if (!sep) return strdup("");        // sin ':' → vacío
    return strdup(sep + 1);             // después del ':'
}


// void aplicarRetardoMemoria() {
//     usleep(configW->retardoMemoria * 1000);
// }


char* ObtenerNombreFileYTag(const char* fileTagText, char** fileOut, char** tagOut) {
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


void ejecutarInstruccion(instruccion_t* instruccion, contexto_query_t* contexto) {
    if (instruccion == NULL) return;
    
    
    log_info(logger, "## Query %d: FETCH - Program Counter: %d - %s", contexto->query_id, contexto->pc, instruccion->parametro[0]);
    
    char* file_Y_tag = instruccion->parametro[1];
    char *fileName = NULL, *tagFile = NULL;
   
    if (instruccion->tipo != END){
        if (!ObtenerNombreFileYTag(file_Y_tag, &fileName, &tagFile)) {
                log_error(logger, "Error al parsear <FILE>:<TAG> en CREATE - Formato inválido: %s", file_Y_tag);
                return;
            } 
    }
    
    switch (instruccion->tipo) {
        case CREATE: { 
            // Formato: CREATE <NOMBRE_FILE>:<TAG>
            // parametro[0] = "CREATE", parametro[1] = "MATERIAS:BASE"
        
            ejecutar_create(fileName, tagFile);

            break;
        }
        
        case TRUNCATE: {
            // Formato: TRUNCATE <NOMBRE_FILE>:<TAG> <TAMAÑO>
            // parametro[0] = "TRUNCATE", parametro[1] = "MATERIAS:BASE", parametro[2] = "1024"
            ejecutar_truncate(fileName, tagFile, atoi(instruccion->parametro[2]));
            
            break;
        }
        
        case WRITE: {
            // Formato: WRITE <NOMBRE_FILE>:<TAG> <DIRECCIÓN BASE> <CONTENIDO>
            // parametro[0] = "WRITE", parametro[1] = "MATERIAS:BASE", parametro[2] = "0", parametro[3] = "SISTEMAS_OPERATIVOS"

            ejecutar_write(fileName, tagFile, atoi(instruccion->parametro[2]), instruccion->parametro[3]);
            
            break;
        }
        
        case READ: {
            // Formato: READ <NOMBRE_FILE>:<TAG> <DIRECCIÓN BASE> <TAMAÑO>
            // parametro[0] = "READ", parametro[1] = "MATERIAS:BASE", parametro[2] = "0", parametro[3] = "8"

            ejecutar_read(fileName, tagFile, atoi(instruccion->parametro[2]), atoi(instruccion->parametro[3]), contexto); 

            break;
        }
        
        case TAG: {
            // Formato: TAG <NOMBRE_FILE_ORIGEN>:<TAG_ORIGEN> <NOMBRE_FILE_DESTINO>:<TAG_DESTINO>
            // parametro[0] = "TAG", parametro[1] = "MATERIAS:BASE", parametro[2] = "MATERIAS:V2"
            char *fileNameDestino = NULL, *tagFileDestino = NULL;
            ObtenerNombreFileYTag(instruccion->parametro[2], &fileNameDestino, &tagFileDestino);

            ejecutar_tag(fileName, tagFile, fileNameDestino, fileNameDestino); 

            free(fileNameDestino);
            free(tagFileDestino);
            break;
        }
        
        case COMMIT: {
            // Formato: COMMIT <NOMBRE_FILE>:<TAG>
            // parametro[0] = "COMMIT", parametro[1] = "MATERIAS:BASE"

            ejecutar_commit(fileName, tagFile);

            break;
        }
        
        case FLUSH: {
            // Formato: FLUSH <NOMBRE_FILE>:<TAG>
            // parametro[0] = "FLUSH", parametro[1] = "MATERIAS:BASE"
            ejecutar_flush();

            break;
        }
        
        case DELETE: {
            // Formato: DELETE <NOMBRE_FILE>:<TAG>
            // parametro[0] = "DELETE", parametro[1] = "MATERIAS:BASE"
            ejecutar_delete(fileName, tagFile); 

            break;
        }
        
        case END: {
            // parametro[0] = "END"
            ejecutar_end(contexto);
            log_debug(logger, "Ejecutando END");
            // implementacion
            break;
        }
        
        case DESCONOCIDO:
        default: {
            log_warning(logger, "Instrucción desconocida en PC %d: %s", contexto->pc, instruccion->parametro[0]);
            break;
        }
    }

    log_info(logger, "## Query %d: - Instrucción realizada: %s", contexto->query_id, instruccion->parametro[0]);

    
    free(fileName);
    free(tagFile);

}

//falta crear el hilo donde que llamara a la funcion y liberara el contexto
void ejecutarQuery(contexto_query_t* contexto) {
    if (contexto == NULL) {
        log_error(logger, "Contexto de query NULL");
        return;
    }
    
    log_debug(logger, "Iniciando ejecución de query %d desde PC %d", contexto->query_id, contexto->pc);
    
    while (contexto->pc < contexto->total_lineas) {
        char* linea_actual = contexto->lineas_query[contexto->pc];
        
        instruccion_t* instruccion = parsearInstruccion(linea_actual);
        if (instruccion != NULL) {
            ejecutarInstruccion(instruccion, contexto);
            
            liberarInstruccion(instruccion);
        }
        
        contexto->pc++;
        
        // if (configW->retardoMemoria > 0) { //esto va en la parte de memoria interna
        //     aplicarRetardoMemoria();
        // }
        free(linea_actual);
    }
    
    log_debug(logger, "Finalizó ejecución de query %d", contexto->query_id);
    
}

void liberarInstruccion(instruccion_t* instr) {
    if (!instr) return;
    for (int i = 0; i < MAX_PARAMETROS; ++i) {
        if (instr->parametro[i] == NULL) break;
        free(instr->parametro[i]);
        instr->parametro[i] = NULL; /* opcional */
    }
    free(instr);
}

void liberarContextoQuery(contexto_query_t* contexto) {
    if (contexto == NULL) return;

    /* free(NULL) es seguro, las comprobaciones son opcionales */
    free(contexto->path_query);

    if (contexto->lineas_query != NULL && contexto->total_lineas > 0) {
        for (int i = 0; i < contexto->total_lineas; i++) {
            free(contexto->lineas_query[i]); /* safe si cada entrada fue inicializada o asignada */
        }
        free(contexto->lineas_query);
    }

    free(contexto);
}