#include "query_interpreter.h"

extern configWorker* configW;
extern t_log* logger;

contexto_query_t* cargarQuery(char* path, int query_id, int pc_inicial) {
    int len = strlen(configW->pathQueries) + strlen(path) + 2;
    char* path_completo = malloc(len);
    snprintf(path_completo, len, "%s/%s", configW->pathQueries, path);
    
    FILE* archivo = fopen(path_completo, "r");
    if (archivo == NULL) {
        log_error(logger, "No se pudo abrir el archivo de query: %s", path_completo);
        return NULL;
    }
    
    contexto_query_t* contexto = malloc(sizeof(contexto_query_t));
    contexto->path_query = strdup(path);
    contexto->query_id = query_id;
    contexto->pc = pc_inicial;
    contexto->total_lineas = 0;
    
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), archivo) != NULL) {
        contexto->total_lineas++;
    }
    
    contexto->lineas_query = malloc(contexto->total_lineas * sizeof(char*));
    
    rewind(archivo);
    int i = 0;
    while (fgets(buffer, sizeof(buffer), archivo) != NULL && i < contexto->total_lineas) {
        buffer[strcspn(buffer, "\n")] = 0;
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
    
    char* linea_copia = strdup(linea);
    
    int espacios = 0;
    for (char* temp = linea_copia; *temp; temp++) {
        if (*temp == ' ') espacios++;
    }
    
    int cantidad_parametros = espacios + 1;
    instruccion->parametros = malloc((cantidad_parametros + 1) * sizeof(char*));
    
    char* token = strtok(linea_copia, " ");
    int i = 0;
    
    while (token != NULL) {
        instruccion->parametros[i] = strdup(token);
        token = strtok(NULL, " ");
        i++;
    }
    
    instruccion->parametros[i] = NULL;
    
    if (i > 0) {
        instruccion->tipo = obtenerTipoInstruccion(instruccion->parametros[0]);
    } else {
        instruccion->tipo = DESCONOCIDO;
    }
    
    free(linea_copia);
    return instruccion;
}

// Función auxiliar para dividir FILE:TAG -- revisar
void dividirFileYTag(char* file_tag, char** nombre_file, char** tag) {
    const char* separador = strchr(file_tag, ':');
    if (separador != NULL) {
        size_t len_nombre = separador - file_tag;
        *nombre_file = strndup(file_tag, len_nombre);
        *tag = strdup(separador + 1);                   
    } else {
        *nombre_file = strdup(file_tag);
        *tag = strdup("");
    }
}

void aplicarRetardoMemoria() {
    usleep(configW->retardoMemoria * 1000);
}

void ejecutarInstruccion(instruccion_t* instruccion, contexto_query_t* contexto) {
    if (instruccion == NULL) return;
    
    char linea_completa[256] = "";
    for (int i = 0; instruccion->parametros[i] != NULL; i++) {
        strcat(linea_completa, instruccion->parametros[i]);
        if (instruccion->parametros[i + 1] != NULL) {
            strcat(linea_completa, " ");
        }
    }
    
    log_info(logger, "## Query %d: FETCH - Program Counter: %d - %s", contexto->query_id, contexto->pc, linea_completa);
    
    switch (instruccion->tipo) {
        case CREATE: {
            // Formato: CREATE <NOMBRE_FILE>:<TAG>
            // parametros[0] = "CREATE", parametros[1] = "MATERIAS:BASE"
            char* file_tag = instruccion->parametros[1];
            char* nombre_file = NULL;
            char* tag = NULL;
            dividirFileYTag(file_tag, &nombre_file, &tag);
            
            log_debug(logger, "Ejecutando CREATE - File: %s, Tag: %s", nombre_file, tag);
            
            // implementacion
            // enviarOpcode(CREATE_FILE, socketStorage);
            // t_paquete* paquete = crearPaquete();
            // agregarStringAPaquete(paquete, nombre_file);
            // agregarStringAPaquete(paquete, tag);
            // enviarPaquete(paquete, socketStorage);
            // eliminarPaquete(paquete);
            
            log_info(logger, "## Query %d: - Instrucción realizada: CREATE", contexto->query_id);
            
            free(nombre_file);
            free(tag);
            break;
        }
        
        case TRUNCATE: {
            // Formato: TRUNCATE <NOMBRE_FILE>:<TAG> <TAMAÑO>
            // parametros[0] = "TRUNCATE", parametros[1] = "MATERIAS:BASE", parametros[2] = "1024"
            char* file_tag = instruccion->parametros[1];
            char* tamanio_str = instruccion->parametros[2];
            char* nombre_file = NULL;
            char* tag = NULL;
            dividirFileYTag(file_tag, &nombre_file, &tag);
            int tamanio = atoi(tamanio_str);
            
            log_debug(logger, "Ejecutando TRUNCATE - File: %s, Tag: %s, Tamaño: %d", nombre_file, tag, tamanio);
            
            // implementacion
            log_info(logger, "## Query %d: - Instrucción realizada: TRUNCATE", contexto->query_id);
            
            free(nombre_file);
            free(tag);
            break;
        }
        
        case WRITE: {
            // Formato: WRITE <NOMBRE_FILE>:<TAG> <DIRECCIÓN BASE> <CONTENIDO>
            // parametros[0] = "WRITE", parametros[1] = "MATERIAS:BASE", parametros[2] = "0", parametros[3] = "SISTEMAS_OPERATIVOS"
            char* file_tag = instruccion->parametros[1];
            char* direccion_str = instruccion->parametros[2];
            char* contenido = instruccion->parametros[3];
            char* nombre_file = NULL;
            char* tag = NULL;
            dividirFileYTag(file_tag, &nombre_file, &tag);
            int direccion_base = atoi(direccion_str);
            
            log_debug(logger, "Ejecutando WRITE - File: %s, Tag: %s, Dir: %d, Contenido: %s", nombre_file, tag, direccion_base, contenido);
            
            // implementacion
            log_info(logger, "## Query %d: - Instrucción realizada: WRITE", contexto->query_id);
            
            free(nombre_file);
            free(tag);
            break;
        }
        
        case READ: {
            // Formato: READ <NOMBRE_FILE>:<TAG> <DIRECCIÓN BASE> <TAMAÑO>
            // parametros[0] = "READ", parametros[1] = "MATERIAS:BASE", parametros[2] = "0", parametros[3] = "8"
            char* file_tag = instruccion->parametros[1];
            char* direccion_str = instruccion->parametros[2];
            char* tamanio_str = instruccion->parametros[3];
            char* nombre_file = NULL;
            char* tag = NULL;
            dividirFileYTag(file_tag, &nombre_file, &tag);
            int direccion_base = atoi(direccion_str);
            int tamanio = atoi(tamanio_str);
            
            log_debug(logger, "Ejecutando READ - File: %s, Tag: %s, Dir: %d, Tamaño: %d", nombre_file, tag, direccion_base, tamanio);
            
            // implementacion
            log_info(logger, "## Query %d: - Instrucción realizada: READ", contexto->query_id);
            
            free(nombre_file);
            free(tag);
            break;
        }
        
        case TAG: {
            // Formato: TAG <NOMBRE_FILE_ORIGEN>:<TAG_ORIGEN> <NOMBRE_FILE_DESTINO>:<TAG_DESTINO>
            // parametros[0] = "TAG", parametros[1] = "MATERIAS:BASE", parametros[2] = "MATERIAS:V2"
            char* file_tag_origen = instruccion->parametros[1];
            char* file_tag_destino = instruccion->parametros[2];
            char* nombre_file_origen = NULL;
            char* tag_origen = NULL;
            char* nombre_file_destino = NULL;
            char* tag_destino = NULL;
            
            dividirFileYTag(file_tag_origen, &nombre_file_origen, &tag_origen);
            dividirFileYTag(file_tag_destino, &nombre_file_destino, &tag_destino);
            
            log_debug(logger, "Ejecutando TAG - Origen: %s:%s, Destino: %s:%s", nombre_file_origen, tag_origen, nombre_file_destino, tag_destino);
            
            // implementacion
            log_info(logger, "## Query %d: - Instrucción realizada: TAG", contexto->query_id);
            
            free(nombre_file_origen);
            free(tag_origen);
            free(nombre_file_destino);
            free(tag_destino);
            break;
        }
        
        case COMMIT: {
            // Formato: COMMIT <NOMBRE_FILE>:<TAG>
            // parametros[0] = "COMMIT", parametros[1] = "MATERIAS:BASE"
            char* file_tag = instruccion->parametros[1];
            char* nombre_file = NULL;
            char* tag = NULL;
            dividirFileYTag(file_tag, &nombre_file, &tag);
            
            log_debug(logger, "Ejecutando COMMIT - File: %s, Tag: %s", nombre_file, tag);
            
            // implementacion
            log_info(logger, "## Query %d: - Instrucción realizada: COMMIT", contexto->query_id);
            
            free(nombre_file);
            free(tag);
            break;
        }
        
        case FLUSH: {
            // Formato: FLUSH <NOMBRE_FILE>:<TAG>
            // parametros[0] = "FLUSH", parametros[1] = "MATERIAS:BASE"
            char* file_tag = instruccion->parametros[1];
            char* nombre_file = NULL;
            char* tag = NULL;
            dividirFileYTag(file_tag, &nombre_file, &tag);
            
            log_debug(logger, "Ejecutando FLUSH - File: %s, Tag: %s", nombre_file, tag);
            
            // implementacion
            log_info(logger, "## Query %d: - Instrucción realizada: FLUSH", contexto->query_id);
            
            free(nombre_file);
            free(tag);
            break;
        }
        
        case DELETE: {
            // Formato: DELETE <NOMBRE_FILE>:<TAG>
            // parametros[0] = "DELETE", parametros[1] = "MATERIAS:BASE"
            char* file_tag = instruccion->parametros[1];
            char* nombre_file = NULL;
            char* tag = NULL;
            dividirFileYTag(file_tag, &nombre_file, &tag);
            
            log_debug(logger, "Ejecutando DELETE - File: %s, Tag: %s", nombre_file, tag);
            
            // implementacion
            log_info(logger, "## Query %d: - Instrucción realizada: DELETE", contexto->query_id);
            
            free(nombre_file);
            free(tag);
            break;
        }
        
        case END: {
            // parametros[0] = "END"
            log_debug(logger, "Ejecutando END");
            log_info(logger, "## Query %d: - Instrucción realizada: END", contexto->query_id);
            // implementacion
            break;
        }
        
        case DESCONOCIDO:
        default: {
            log_warning(logger, "Instrucción desconocida en PC %d: %s", contexto->pc, linea_completa);
            break;
        }
    }
}

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
            
            if (instruccion->tipo == END) {
                liberarInstruccion(instruccion);
                break;
            }
            
            liberarInstruccion(instruccion);
        }
        
        contexto->pc++;
        
        if (configW->retardoMemoria > 0) {
            aplicarRetardoMemoria();
        }
    }
    
    log_debug(logger, "Finalizó ejecución de query %d", contexto->query_id);
}

void liberarInstruccion(instruccion_t* instruccion) {
    if (instruccion != NULL) {
        if (instruccion->parametros != NULL) {
            for (int i = 0; instruccion->parametros[i] != NULL; i++) {
                free(instruccion->parametros[i]);
            }
            free(instruccion->parametros);
        }
        free(instruccion);
    }
}

void liberarContextoQuery(contexto_query_t* contexto) {
    if (contexto != NULL) {
        free(contexto->path_query);
        
        if (contexto->lineas_query != NULL) {
            for (int i = 0; i < contexto->total_lineas; i++) {
                free(contexto->lineas_query[i]);
            }
            free(contexto->lineas_query);
        }
        
        free(contexto);
    }
}