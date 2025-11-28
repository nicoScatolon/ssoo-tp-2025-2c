#include "metadata.h"

void crearMetaData(char* pathTag){
    inicializarArchivo(pathTag,"metadata","config","a+");
    inicializarMetaData(pathTag);
    
}

void inicializarMetaData(char* pathTag) {
    char* pathMetadata = string_from_format("%s/metadata.config", pathTag);

    FILE* archivo = fopen(pathMetadata, "w+");
    if (archivo == NULL) {
        perror("Error al crear metadata.config");
        free(pathMetadata);
        return;
    }

    int tamanoInicial = 0;
    char* estadoInicial = "WORK_IN_PROGRESS";
    char* bloquesIniciales = "[]";

    fprintf(archivo, "TAMAÑO=%d\n", tamanoInicial);
    fprintf(archivo, "BLOQUES=%s\n", bloquesIniciales);
    fprintf(archivo, "ESTADO=%s\n", estadoInicial);

    fclose(archivo);
    free(pathMetadata);
}

void cambiarEstadoMetaData(char* pathTag,char* estado) {
    char* pathMetadata = string_from_format("%s/metadata.config", pathTag);

    t_config* config = config_create(pathMetadata);
    if (config == NULL) {
        fprintf(stderr, "Error: no se pudo abrir %s\n", pathMetadata);
        free(pathMetadata);
        return;
    }
    config_set_value(config, "ESTADO", estado);
    config_save(config);
    config_destroy(config);
    free(pathMetadata);
}

// void agregarBloqueMetaData1(char* pathTag, int bloqueLogico,int nuevoBloqueFisico) {
//     char* pathMetadata = string_from_format("%s/metadata.config", pathTag);
    
//     t_config* config = config_create(pathMetadata);
//     if (config == NULL) {
//         log_error(logger, "Error al abrir metadata en: %s", pathMetadata);
//         free(pathMetadata);
//         exit(EXIT_FAILURE);
//     }
    
//     // Obtener el string del array y parsearlo
//     char* bloquesString = config_get_string_value(config, "BLOQUES");
//     char** bloquesActuales = NULL;
//     int cantidadBloques = 0;
    
//     if (bloquesString != NULL && strlen(bloquesString) > 0) {
//         // Usar string_get_string_as_array para parsear el string "[1,2,3]"
//         bloquesActuales = string_get_string_as_array(bloquesString);
        
//         // Contar bloques existentes
//         if (bloquesActuales != NULL) {
//             while (bloquesActuales[cantidadBloques] != NULL) {
//                 cantidadBloques++;
//             }
//         }
//     }
    
//     // Crear array para incluir el nuevo bloque
//     int* bloques = malloc(sizeof(int) * (cantidadBloques + 1));
    
//     // Copiar bloques existentes al array de enteros
//     for (int i = 0; i < cantidadBloques; i++) {
//         bloques[i] = atoi(bloquesActuales[i]);
//     }
    
//     // Agregar el nuevo bloque
//     bloques[bloqueLogico] = nuevoBloqueFisico;
    

//     // Construir el string con los bloques ordenados
//     char* nuevoValor = string_new();
//     string_append(&nuevoValor, "[");
//     for (int i = 0; i < cantidadBloques; i++) {
//         char* numStr = string_from_format("%d", bloques[i]);
//         string_append(&nuevoValor, numStr);
//         if (i < cantidadBloques - 1) {
//             string_append(&nuevoValor, ",");
//         }
//         free(numStr);
//     }
//     string_append(&nuevoValor, "]");
    
//     // Guardar y limpiar
//     config_set_value(config, "BLOQUES", nuevoValor);
//     config_save(config);
    
//     // Liberar memoria
//     if (bloquesActuales != NULL) {
//         string_iterate_lines(bloquesActuales, (void*)free);
//         free(bloquesActuales);
//     }
//     free(bloques);
//     free(nuevoValor);
//     free(pathMetadata);
//     config_destroy(config);
    
//     log_debug(logger, "Bloque %d agregado en la posicion <%d> a metadata en %s", nuevoBloqueFisico, bloqueLogico, pathTag);
// }

void agregarBloqueMetaData(char* pathTag, int bloqueLogico, int nuevoBloqueFisico) {
    char* pathMetadata = string_from_format("%s/metadata.config", pathTag);
    t_config* config = config_create(pathMetadata);
    
    if (config == NULL) {
        log_error(logger, "Error al abrir metadata en: %s", pathMetadata);
        free(pathMetadata);
        exit(EXIT_FAILURE);
    }
    
    // Obtener el string del array y parsearlo
    char* bloquesString = config_get_string_value(config, "BLOQUES");
    char** bloquesActuales = NULL;
    int cantidadBloques = 0;
    
    if (bloquesString != NULL && strlen(bloquesString) > 0) {
        bloquesActuales = string_get_string_as_array(bloquesString);
        if (bloquesActuales != NULL) {
            while (bloquesActuales[cantidadBloques] != NULL) {
                cantidadBloques++;
            }
        }
    }
    
    // Crear array para incluir el nuevo bloque
    int nuevaCantidad = (bloqueLogico >= cantidadBloques) ? (bloqueLogico + 1) : cantidadBloques;
    int* bloques = malloc(sizeof(int) * nuevaCantidad);
    
    // Copiar bloques existentes
    for (int i = 0; i < cantidadBloques; i++) {
        bloques[i] = atoi(bloquesActuales[i]);
    }
    
    // Agregar el nuevo bloque en la posición indicada
    bloques[bloqueLogico] = nuevoBloqueFisico;
    
    // Construir el string con los bloques
    char* nuevoValor = string_new();
    string_append(&nuevoValor, "[");
    
    for (int i = 0; i < nuevaCantidad; i++) {  // CAMBIO CLAVE: iterar hasta nuevaCantidad
        char* numStr = string_from_format("%d", bloques[i]);
        string_append(&nuevoValor, numStr);
        if (i < nuevaCantidad - 1) {  // CAMBIO CLAVE: comparar con nuevaCantidad
            string_append(&nuevoValor, ",");
        }
        free(numStr);
    }
    
    string_append(&nuevoValor, "]");
    
    // Guardar y limpiar
    config_set_value(config, "BLOQUES", nuevoValor);
    config_save(config);
    
    // Liberar memoria
    if (bloquesActuales != NULL) {
        string_iterate_lines(bloquesActuales, (void*)free);
        free(bloquesActuales);
    }
    free(bloques);
    free(nuevoValor);
    free(pathMetadata);
    config_destroy(config);
    
    log_debug(logger, "Bloque %d agregado en la posicion <%d> a metadata en %s", 
              nuevoBloqueFisico, bloqueLogico, pathTag);
}