#include "operaciones.h"

char* lecturaBloque(char* file, char* tag, int numeroBloque){
    char rutaCompleta[512];
    snprintf(rutaCompleta, sizeof(rutaCompleta), 
             "%s/pathfiles/%s/%s/logical_blocks/%d.dat", 
             configS->puntoMontaje, file, tag, numeroBloque);


    FILE* bloque = fopen(rutaCompleta, "r");
    if (bloque == NULL) {
        log_debug(logger,"No Existe el bloque logico numero <%d> asociado al file:tag <%s:%s>\n", numeroBloque,file,tag);
        return NULL;
    }
    char* contenido = (char*)malloc((configSB->BLOCK_SIZE + 1) * sizeof(char));
    size_t bytesLeidos = fread(contenido, sizeof(char), configSB->BLOCK_SIZE, bloque);
    contenido[bytesLeidos] = '\0';
    fclose(bloque);
    usleep(configS->retardoAccesoBloque*1000);
    log_debug(logger,"Contenido del bloque <%d> asociado al file:tag <%s:%s>: %s",numeroBloque,file,tag,contenido);
    return contenido;
}


// Tag de File
void tagFile(char* fileOrigen, char* tagOrigen, char* fileDestino, char* tagDestino, int queryId) {
    char pathTagOrigen[512];
    char pathFileDestino[512];
    char pathTagDestino[512];
    
    snprintf(pathTagOrigen, sizeof(pathTagOrigen), 
             "%s/files/%s/%s", configS->puntoMontaje, fileOrigen, tagOrigen);
    
    snprintf(pathFileDestino, sizeof(pathFileDestino), 
             "%s/files/%s", configS->puntoMontaje, fileDestino);
    
    snprintf(pathTagDestino, sizeof(pathTagDestino), 
             "%s/files/%s/%s", configS->puntoMontaje, fileDestino, tagDestino);
    
    // Verificar que el tag origen existe
    struct stat st;
    if (stat(pathTagOrigen, &st) != 0) {
        log_error(logger, "##<%d> - Error: Tag origen %s:%s no existe", 
                  queryId, fileOrigen, tagOrigen);
        return;
    }
    
    // Verificar que el tag destino NO existe
    if (stat(pathTagDestino, &st) == 0) {
        log_error(logger, "##<%d> - Error: Tag destino %s:%s ya existe", 
                  queryId, fileDestino, tagDestino);
        return;
    }
    
    // Crear directorio del file destino si no existe
    if (stat(pathFileDestino, &st) != 0) {
        if (mkdir(pathFileDestino, 0777) != 0) {
            log_error(logger, "##<%d> - Error al crear directorio del file destino: %s", 
                      queryId, fileDestino);
            return;
        }
    }
    
    // Copiar el directorio completo del tag origen al destino
    if (!copiarDirectorioRecursivo(pathTagOrigen, pathTagDestino)) {
        log_error(logger, "##<%d> - Error al copiar directorio de %s:%s a %s:%s", 
                  queryId, fileOrigen, tagOrigen, fileDestino, tagDestino);
        return;
    }
    
    // Modificar el estado del metadata del tag destino a WORK_IN_PROGRESS
    char pathMetadataDestino[512];
    snprintf(pathMetadataDestino, sizeof(pathMetadataDestino), 
             "%s/metadata.config", pathTagDestino);
    
    t_config* configMetadata = config_create(pathMetadataDestino);
    if (configMetadata == NULL) {
        log_error(logger, "##<%d> - Error al abrir metadata de %s:%s", 
                  queryId, fileDestino, tagDestino);
        return;
    }
    
    config_set_value(configMetadata, "ESTADO", "WORK_IN_PROGRESS");
    config_save(configMetadata);
    config_destroy(configMetadata);
    
    // Aplicar retardos
    usleep(configS->retardoOperacion * 1000);
    
    log_info(logger, "##<%d> - Tag creado <%s:%s>", queryId, fileDestino, tagDestino);
}

bool copiarDirectorioRecursivo(const char* origen, const char* destino) {
    // Crear el directorio destino
    if (mkdir(destino, 0777) != 0 && errno != EEXIST) {
        log_error(logger, "Error al crear directorio destino: %s - %s", 
                  destino, strerror(errno));
        return false;
    }
    
    DIR* dir = opendir(origen);
    if (dir == NULL) {
        log_error(logger, "Error al abrir directorio origen: %s - %s", 
                  origen, strerror(errno));
        return false;
    }
    
    struct dirent* entrada;
    while ((entrada = readdir(dir)) != NULL) {
        // Ignorar . y ..
        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0) {
            continue;
        }
        
        char pathOrigen[512];
        char pathDestino[512];
        snprintf(pathOrigen, sizeof(pathOrigen), "%s/%s", origen, entrada->d_name);
        snprintf(pathDestino, sizeof(pathDestino), "%s/%s", destino, entrada->d_name);
        
        struct stat st;
        if (stat(pathOrigen, &st) != 0) {
            log_error(logger, "Error al obtener info de: %s", pathOrigen);
            continue;
        }
        
        if (S_ISDIR(st.st_mode)) {
            // Es un directorio, copiar recursivamente
            if (!copiarDirectorioRecursivo(pathOrigen, pathDestino)) {
                closedir(dir);
                return false;
            }
        } else {
            // Es un archivo regular, copiarlo
            if (!copiarArchivo(pathOrigen, pathDestino)) {
                closedir(dir);
                return false;
            }
        }
    }
    
    closedir(dir);
    return true;
}

bool copiarArchivo(const char* origen, const char* destino) {
    FILE* archivoOrigen = fopen(origen, "rb");
    if (archivoOrigen == NULL) {
        log_error(logger, "Error al abrir archivo origen: %s - %s", 
                  origen, strerror(errno));
        return false;
    }
    
    FILE* archivoDestino = fopen(destino, "wb");
    if (archivoDestino == NULL) {
        log_error(logger, "Error al crear archivo destino: %s - %s", 
                  destino, strerror(errno));
        fclose(archivoOrigen);
        return false;
    }
    
    char buffer[4096];
    size_t bytesLeidos;
    
    while ((bytesLeidos = fread(buffer, 1, sizeof(buffer), archivoOrigen)) > 0) {
        if (fwrite(buffer, 1, bytesLeidos, archivoDestino) != bytesLeidos) {
            log_error(logger, "Error al escribir en archivo destino: %s", destino);
            fclose(archivoOrigen);
            fclose(archivoDestino);
            return false;
        }
    }
    
    fclose(archivoOrigen);
    fclose(archivoDestino);
    
    // Copiar permisos
    struct stat st;
    if (stat(origen, &st) == 0) {
        chmod(destino, st.st_mode);
    }
    
    return true;
}