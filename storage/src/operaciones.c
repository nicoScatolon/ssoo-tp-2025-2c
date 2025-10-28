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
    aplicarRetardoBloque();
    log_debug(logger,"Contenido del bloque <%d> asociado al file:tag <%s:%s>: %s",numeroBloque,file,tag,contenido);
    return contenido;
}


// Tag de File
bool tagFile(char* fileOrigen, char* tagOrigen, char* fileDestino, char* tagDestino, int queryID){
    char pathTagOrigen[512];
    char pathFileDestino[512];
    char pathTagDestino[512];
    
    snprintf(pathTagOrigen, sizeof(pathTagOrigen), 
             "%s/files/%s/%s", configS->puntoMontaje, fileOrigen, tagOrigen);
    
    snprintf(pathFileDestino, sizeof(pathFileDestino), 
             "%s/files/%s", configS->puntoMontaje, fileDestino);
    
    snprintf(pathTagDestino, sizeof(pathTagDestino), 
             "%s/files/%s/%s", configS->puntoMontaje, fileDestino, tagDestino);
    
    struct stat st;

    // Verificacion de existncia del TAG ORIGEN (No existe)
    if (stat(pathTagOrigen, &st) != 0) {
        log_debug(logger, "## <%d>- Error: Tag origen <%s:%s> no existe",queryID,fileOrigen, tagOrigen);
        return false;
    }
    
    // Verificando de existencia del TAG DESTINO (Ya existe)
    if (stat(pathTagDestino, &st) == 0) {
        log_debug(logger, "## <%d>- Error: Tag destino <%s:%s> ya existe", queryID,fileDestino, tagDestino);
        return false;
    }
    
    if (stat(pathFileDestino, &st) != 0) {
        if (mkdir(pathFileDestino, 0777) != 0) {
            log_error(logger, "## <%d> - Error al crear directorio del file destino: %s", queryID,fileDestino);
            exit(EXIT_FAILURE);
        }
    }
    
    // Copiar el directorio completo del tag origen al destino
    if (!copiarDirectorioRecursivo(pathTagOrigen, pathTagDestino)) {
        log_error(logger, "##<%d> - Error al copiar directorio de %s:%s a %s:%s",queryID, fileOrigen, tagOrigen, fileDestino, tagDestino);
        exit(EXIT_FAILURE);
    }
    
    cambiarEstadoMetaData(pathTagDestino,"WORK_IN_PROGRESS");
    return true;
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

void aplicarRetardoBloque(){
    usleep(configS->retardoAccesoBloque*1000);
}

void aplicarRetardoOperacion(){
    usleep(configS->retardoOperacion*1000);
}










// Planteando WRITE, TRUNCATE, DELETE, COMMIT



// WRITE:
bool escribirBloque(char* file, char* tag, int numeroBloqueLogico, char* contenido, int queryID) {
    
    // 1. Construir path al metadata del File:Tag
    char pathMetadata[512];
    snprintf(pathMetadata, sizeof(pathMetadata), 
             "%s/files/%s/%s/metadata.config", 
             configS->puntoMontaje, file, tag);

    // Verificar existencia del File:Tag
    if (access(pathMetadata, F_OK) != 0) {
        log_error(logger, "##<%d> - Error: File:Tag <%s:%s> no existe", queryID, file, tag);
        return false;
    }

    // 2. Leer metadata y verificar estado
    t_config* metadata = config_create(pathMetadata);
    if (!metadata) {
        log_error(logger, "##<%d> - Error al abrir metadata de <%s:%s>", queryID, file, tag);
        return false;
    }

    char* estado = config_get_string_value(metadata, "ESTADO");
    if (strcmp(estado, "COMMITTED") == 0) {
        log_error(logger, "##<%d> - Error: No se puede escribir en <%s:%s> (COMMITTED)", 
                  queryID, file, tag);
        config_destroy(metadata);
        return false;
    }

    // 3. Verificar que el bloque lógico esté asignado
    char** bloques = config_get_array_value(metadata, "BLOCKS");
    int cantidadBloques = 0;
    while (bloques[cantidadBloques] != NULL) cantidadBloques++;

    if (numeroBloqueLogico < 0 || numeroBloqueLogico >= cantidadBloques) {
        log_error(logger, "##<%d> - Error: Bloque lógico <%d> no asignado en <%s:%s>", 
                  queryID, numeroBloqueLogico, file, tag);
        config_destroy(metadata);
        return false;
    }

    int bloqueFisicoActual = atoi(bloques[numeroBloqueLogico]);
    
    // 4. Contar referencias al bloque físico usando stat()
    char pathBloqueLogico[512];
    snprintf(pathBloqueLogico, sizeof(pathBloqueLogico),
             "%s/files/%s/%s/logical_blocks/%06d.dat",
             configS->puntoMontaje, file, tag, numeroBloqueLogico);

    struct stat st;
    if (stat(pathBloqueLogico, &st) != 0) {
        log_error(logger, "##<%d> - Error al obtener info del bloque lógico <%d>", 
                  queryID, numeroBloqueLogico);
        config_destroy(metadata);
        return false;
    }

    int cantidadReferencias = st.st_nlink;

    // 5. Caso 1: Única referencia → Escribir directamente
    if (cantidadReferencias == 1) {
        char pathBloqueFisico[512];
        snprintf(pathBloqueFisico, sizeof(pathBloqueFisico),
                 "%s/physical_blocks/block%04d.dat",
                 configS->puntoMontaje, bloqueFisicoActual);

        FILE* archivo = fopen(pathBloqueFisico, "r+b");
        if (!archivo) {
            log_error(logger, "##<%d> - Error al abrir bloque físico <%d>", 
                      queryID, bloqueFisicoActual);
            config_destroy(metadata);
            return false;
        }

        fwrite(contenido, 1, configSB->BLOCK_SIZE, archivo);
        fclose(archivo);

        // Actualizar hash
        actualizarHashBloque(bloqueFisicoActual, contenido, queryID);

        log_info(logger, "##<%d> - Bloque Lógico Escrito <%s:%s> - Número de Bloque: <%d>",
                 queryID, file, tag, numeroBloqueLogico);
        
        config_destroy(metadata);
        return true;
    }

    // 6. Caso 2: Múltiples referencias → Copy-on-Write
    log_debug(logger, "##<%d> - Bloque físico <%d> tiene <%d> referencias. Aplicando Copy-on-Write",
              queryID, bloqueFisicoActual, cantidadReferencias);

    // Buscar bloque físico libre
    ssize_t nuevoBloqueFisico = bitmapReservarLibre();
    if (nuevoBloqueFisico < 0) {
        log_error(logger, "##<%d> - Error: No hay bloques físicos disponibles", queryID);
        config_destroy(metadata);
        return false;
    }

    // Escribir contenido en el nuevo bloque físico
    char pathNuevoBloqueFisico[512];
    snprintf(pathNuevoBloqueFisico, sizeof(pathNuevoBloqueFisico),
             "%s/physical_blocks/block%04d.dat",
             configS->puntoMontaje, (int)nuevoBloqueFisico);

    FILE* archivo = fopen(pathNuevoBloqueFisico, "w+b");
    if (!archivo) {
        log_error(logger, "##<%d> - Error al crear nuevo bloque físico <%ld>", 
                  queryID, nuevoBloqueFisico);
        liberarBloque((int)nuevoBloqueFisico);
        config_destroy(metadata);
        return false;
    }

    fwrite(contenido, 1, configSB->BLOCK_SIZE, archivo);
    fclose(archivo);

    // Agregar hash del nuevo bloque
    char* hash = crearHash(contenido);
    escribirHash(hash, (int)nuevoBloqueFisico);
    free(hash);

    // 7. Eliminar hardlink anterior y crear nuevo
    if (unlink(pathBloqueLogico) != 0) {
        log_error(logger, "##<%d> - Error al eliminar hardlink del bloque lógico <%d>",
                  queryID, numeroBloqueLogico);
        config_destroy(metadata);
        return false;
    }

    log_info(logger, "##<%d> - <%s:%s> Se eliminó el hard link del bloque lógico <%d> al bloque físico <%d>",
             queryID, file, tag, numeroBloqueLogico, bloqueFisicoActual);

    if (link(pathNuevoBloqueFisico, pathBloqueLogico) != 0) {
        log_error(logger, "##<%d> - Error al crear hardlink al nuevo bloque físico <%ld>",
                  queryID, nuevoBloqueFisico);
        config_destroy(metadata);
        return false;
    }

    log_info(logger, "##<%d> - <%s:%s> Se agregó el hard link del bloque lógico <%d> al bloque físico <%ld>",
             queryID, file, tag, numeroBloqueLogico, nuevoBloqueFisico);

    // 8. Actualizar metadata con nuevo bloque físico
    bloques[numeroBloqueLogico] = string_itoa((int)nuevoBloqueFisico);
    
    char* nuevosBloquesStr = string_new();
    string_append(&nuevosBloquesStr, "[");
    for (int i = 0; i < cantidadBloques; i++) {
        string_append(&nuevosBloquesStr, bloques[i]);
        if (i < cantidadBloques - 1) string_append(&nuevosBloquesStr, ",");
    }
    string_append(&nuevosBloquesStr, "]");

    config_set_value(metadata, "BLOCKS", nuevosBloquesStr);
    config_save(metadata);
    free(nuevosBloquesStr);

    // 9. Verificar si el bloque físico antiguo quedó sin referencias
    char pathBloqueAntiguo[512];
    snprintf(pathBloqueAntiguo, sizeof(pathBloqueAntiguo),
             "%s/physical_blocks/block%04d.dat",
             configS->puntoMontaje, bloqueFisicoActual);

    struct stat stAntiguo;
    if (stat(pathBloqueAntiguo, &stAntiguo) == 0 && stAntiguo.st_nlink == 1) {
        liberarBloque(bloqueFisicoActual);
        log_info(logger, "##<%d> - Bloque Físico Liberado - Número de Bloque: <%d>",
                 queryID, bloqueFisicoActual);
    }

    log_info(logger, "##<%d> - Bloque Lógico Escrito <%s:%s> - Número de Bloque: <%d>",
             queryID, file, tag, numeroBloqueLogico);
    log_info(logger, "##<%d> - Bloque Físico Reservado - Número de Bloque: <%ld>",
             queryID, nuevoBloqueFisico);

    config_destroy(metadata);
    return true;
}

void actualizarHashBloque(int numeroBloqueFisico, char* contenido, int queryID) {
    char* nuevoHash = crearHash(contenido);
    
    // Eliminar hash anterior del archivo blocks_hash_index.config
    char archivoHashPath[512];
    snprintf(archivoHashPath, sizeof(archivoHashPath), 
             "%s/blocks_hash_index.config", configS->puntoMontaje);

    pthread_mutex_lock(&mutex_hash_block);

    t_config* configHash = config_create(archivoHashPath);
    if (!configHash) {
        log_error(logger, "##<%d> - Error al abrir blocks_hash_index.config", queryID);
        free(nuevoHash);
        pthread_mutex_unlock(&mutex_hash_block);
        return;
    }

    // Buscar y eliminar el hash anterior asociado a este bloque
    // char** keys = config_keys(configHash);
    // for (int i = 0; keys[i] != NULL; i++) {
    //     char* valor = config_get_string_value(configHash, keys[i]);
    //     char bloqueEsperado[16];
    //     snprintf(bloqueEsperado, sizeof(bloqueEsperado), "block%04d", numeroBloqueFisico);
        
    //     if (strcmp(valor, bloqueEsperado) == 0) {
    //         config_remove_key(configHash, keys[i]);
    //         break;
    //     }
    // }

    config_destroy(configHash);

    // Escribir el nuevo hash
    escribirHash(nuevoHash, numeroBloqueFisico);
    
    log_debug(logger, "##<%d> - Hash actualizado para bloque físico <%d>", 
              queryID, numeroBloqueFisico);

    free(nuevoHash);
    pthread_mutex_unlock(&mutex_hash_block);
}



// TRUNCATE:



bool truncarArchivo(char* file, char* tag, int nuevoTamanio, int queryID) {
    
    // Validar que el tamaño sea múltiplo del tamaño de bloque
    if (nuevoTamanio % configSB->BLOCK_SIZE != 0) {
        log_error(logger, "##<%d> - Error: El tamaño <%d> no es múltiplo del tamaño de bloque <%d>",
                  queryID, nuevoTamanio, configSB->BLOCK_SIZE);
        return false;
    }

    // Obtener metadata
    t_config* metadata = obtenerMetadata(file, tag, queryID);
    if (!metadata) {
        return false;
    }

    // Verificar que no esté COMMITTED
    char* estado = config_get_string_value(metadata, "ESTADO");
    if (strcmp(estado, "COMMITTED") == 0) {
        log_error(logger, "##<%d> - Error: No se puede truncar <%s:%s> (COMMITTED)", 
                  queryID, file, tag);
        config_destroy(metadata);
        return false;
    }

    // Obtener tamaño actual
    int tamanioActual = config_get_int_value(metadata, "TAMAÑO");
    int bloquesActuales = tamanioActual / configSB->BLOCK_SIZE;
    int bloquesNuevos = nuevoTamanio / configSB->BLOCK_SIZE;

    log_debug(logger, "##<%d> - Truncando <%s:%s>: %d bytes (%d bloques) -> %d bytes (%d bloques)",
              queryID, file, tag, tamanioActual, bloquesActuales, nuevoTamanio, bloquesNuevos);

    bool resultado = false;

    if (bloquesNuevos > bloquesActuales) {
        // Aumentar tamaño
        resultado = aumentarTamanioArchivo(file, tag, bloquesActuales, bloquesNuevos, 
                                           nuevoTamanio, metadata, queryID);
    } else if (bloquesNuevos < bloquesActuales) {
        // Reducir tamaño
        resultado = reducirTamanioArchivo(file, tag, bloquesActuales, bloquesNuevos, 
                                          nuevoTamanio, metadata, queryID);
    } else {
        // Mismo tamaño, solo actualizar metadata
        config_set_value(metadata, "TAMAÑO", string_itoa(nuevoTamanio));
        config_save(metadata);
        resultado = true;
        
        log_info(logger, "##<%d> - File Truncado <%s:%s> - Tamaño: <%d>",
                 queryID, file, tag, nuevoTamanio);
    }

    config_destroy(metadata);
    return resultado;
}

t_config* obtenerMetadata(char* file, char* tag, int queryID) {
    char pathMetadata[512];
    snprintf(pathMetadata, sizeof(pathMetadata), 
             "%s/files/%s/%s/metadata.config", 
             configS->puntoMontaje, file, tag);

    if (access(pathMetadata, F_OK) != 0) {
        log_error(logger, "##<%d> - Error: File:Tag <%s:%s> no existe", 
                  queryID, file, tag);
        return NULL;
    }

    t_config* metadata = config_create(pathMetadata);
    if (!metadata) {
        log_error(logger, "##<%d> - Error al abrir metadata de <%s:%s>", 
                  queryID, file, tag);
        return NULL;
    }

    return metadata;
}

bool aumentarTamanioArchivo(char* file, char* tag, int bloquesActuales, int bloquesNuevos,
                            int nuevoTamanio, t_config* metadata, int queryID) {
    
    int bloquesAAgregar = bloquesNuevos - bloquesActuales;

    log_debug(logger, "##<%d> - Aumentando tamaño: agregar %d bloques lógicos",
              queryID, bloquesAAgregar);

    // Verificar espacio disponible en el bitmap
    if (!hayEspacioDisponible(bloquesAAgregar, queryID)) {
        log_error(logger, "##<%d> - Error: Espacio insuficiente (se necesitan %d bloques)",
                  queryID, bloquesAAgregar);
        return false;
    }

    // Construir path al bloque físico 0
    char pathBloqueFisico0[512];
    snprintf(pathBloqueFisico0, sizeof(pathBloqueFisico0),
             "%s/physical_blocks/block0000.dat",
             configS->puntoMontaje);

    // Construir path a logical_blocks
    char pathLogicalBlocks[512];
    snprintf(pathLogicalBlocks, sizeof(pathLogicalBlocks),
             "%s/files/%s/%s/logical_blocks",
             configS->puntoMontaje, file, tag);

    // Crear hardlinks apuntando al bloque 0
    for (int i = bloquesActuales; i < bloquesNuevos; i++) {
        char pathBloqueLogico[512];
        snprintf(pathBloqueLogico, sizeof(pathBloqueLogico),
                 "%s/%06d.dat", pathLogicalBlocks, i);

        if (link(pathBloqueFisico0, pathBloqueLogico) != 0) {
            log_error(logger, "##<%d> - Error al crear hardlink para bloque lógico %d: %s",
                      queryID, i, strerror(errno));
            return false;
        }

        log_info(logger, "##<%d> - <%s:%s> Se agregó el hard link del bloque lógico <%d> al bloque físico <0>",
                 queryID, file, tag, i);
    }

    // Actualizar metadata (tamaño y bloques)
    actualizarMetadataTruncate(metadata, nuevoTamanio, bloquesNuevos);

    log_info(logger, "##<%d> - File Truncado <%s:%s> - Tamaño: <%d>",
             queryID, file, tag, nuevoTamanio);

    return true;
}

int contarReferenciasBloque(char* pathBloqueLogico, int queryID) {
    struct stat st;
    
    if (stat(pathBloqueLogico, &st) != 0) {
        log_error(logger, "##<%d> - Error al obtener info del bloque lógico: %s", 
                  queryID, strerror(errno));
        return -1;
    }

    return st.st_nlink;
}

bool reducirTamanioArchivo(char* file, char* tag, int bloquesActuales, int bloquesNuevos,
                           int nuevoTamanio, t_config* metadata, int queryID) {
    
    int bloquesAEliminar = bloquesActuales - bloquesNuevos;

    log_debug(logger, "##<%d> - Reduciendo tamaño: eliminar %d bloques lógicos (desde el final)",
              queryID, bloquesAEliminar);

    char** bloques = config_get_array_value(metadata, "BLOCKS");

    // Eliminar bloques desde el final
    for (int i = bloquesActuales - 1; i >= bloquesNuevos; i--) {
        int bloqueFisico = atoi(bloques[i]);

        // Construir path del bloque lógico
        char pathBloqueLogico[512];
        snprintf(pathBloqueLogico, sizeof(pathBloqueLogico),
                 "%s/files/%s/%s/logical_blocks/%06d.dat",
                 configS->puntoMontaje, file, tag, i);

        // Verificar referencias antes de eliminar
        int referencias = contarReferenciasBloque(pathBloqueLogico, queryID);
        if (referencias < 0) {
            log_error(logger, "##<%d> - Error al obtener referencias del bloque lógico %d",
                      queryID, i);
            return false;
        }

        // Eliminar hardlink
        if (unlink(pathBloqueLogico) != 0) {
            log_error(logger, "##<%d> - Error al eliminar bloque lógico %d: %s",
                      queryID, i, strerror(errno));
            return false;
        }

        log_info(logger, "##<%d> - <%s:%s> Se eliminó el hard link del bloque lógico <%d> al bloque físico <%d>",
                 queryID, file, tag, i, bloqueFisico);

        // Si era la única referencia, liberar el bloque físico
        if (referencias == 1) {
            liberarBloque(bloqueFisico);
            log_info(logger, "##<%d> - Bloque Físico Liberado - Número de Bloque: <%d>",
                     queryID, bloqueFisico);
        }
    }

    // Actualizar metadata (tamaño y bloques)
    actualizarMetadataTruncate(metadata, nuevoTamanio, bloquesNuevos);

    log_info(logger, "##<%d> - File Truncado <%s:%s> - Tamaño: <%d>",
             queryID, file, tag, nuevoTamanio);

    return true;
}

bool hayEspacioDisponible(int bloquesNecesarios, int queryID) {
    pthread_mutex_lock(&mutex_bitmap_file);

    if (!bitmap) {
        log_error(logger, "##<%d> - Bitmap no inicializado", queryID);
        pthread_mutex_unlock(&mutex_bitmap_file);
        return false;
    }

    int bloquesLibres = 0;
    for (off_t i = 0; i < (off_t)bitmap_num_bits; i++) {
        if (!bitarray_test_bit(bitmap, i)) {
            bloquesLibres++;
            if (bloquesLibres >= bloquesNecesarios) {
                pthread_mutex_unlock(&mutex_bitmap_file);
                return true;
            }
        }
    }

    pthread_mutex_unlock(&mutex_bitmap_file);
    
    log_debug(logger, "##<%d> - Espacio insuficiente: se necesitan %d bloques, disponibles %d",
              queryID, bloquesNecesarios, bloquesLibres);
    
    return false;
}

void actualizarMetadataTruncate(t_config* metadata, int nuevoTamanio, int cantidadBloques) {
    
    // Actualizar tamaño
    config_set_value(metadata, "TAMAÑO", string_itoa(nuevoTamanio));

    // Reconstruir array de bloques (0 a cantidadBloques-1)
    char* nuevosBloquesStr = string_new();
    string_append(&nuevosBloquesStr, "[");

    for (int i = 0; i < cantidadBloques; i++) {
        char* numStr = string_itoa(i);
        string_append(&nuevosBloquesStr, numStr);
        
        if (i < cantidadBloques - 1) {
            string_append(&nuevosBloquesStr, ",");
        }
        
        free(numStr);
    }

    string_append(&nuevosBloquesStr, "]");

    config_set_value(metadata, "BLOCKS", nuevosBloquesStr);
    config_save(metadata);

    free(nuevosBloquesStr);
}



// DELETE:



bool eliminarTag(char* file, char* tag, int queryID) {
    
    // Verificar existencia del File:Tag
    char pathTag[512];
    snprintf(pathTag, sizeof(pathTag), 
             "%s/files/%s/%s", 
             configS->puntoMontaje, file, tag);

    if (access(pathTag, F_OK) != 0) {
        log_error(logger, "##<%d> - Error: File:Tag <%s:%s> no existe", 
                  queryID, file, tag);
        return false;
    }

    // Obtener metadata para leer los bloques asignados
    t_config* metadata = obtenerMetadata(file, tag, queryID);
    if (!metadata) {
        return false;
    }

    // Obtener array de bloques
    char** bloques = config_get_array_value(metadata, "BLOCKS");
    if (!bloques) {
        log_warning(logger, "##<%d> - <%s:%s> no tiene bloques asignados", 
                    queryID, file, tag);
        config_destroy(metadata);
        // Continuar con la eliminación del directorio
    } else {
        // Procesar cada bloque lógico antes de eliminar
        int cantidadBloques = 0;
        while (bloques[cantidadBloques] != NULL) {
            cantidadBloques++;
        }

        log_debug(logger, "##<%d> - <%s:%s> tiene %d bloques asignados. Verificando referencias...",
                  queryID, file, tag, cantidadBloques);

        // Liberar bloques físicos si corresponde
        for (int i = 0; i < cantidadBloques; i++) {
            int bloqueFisico = atoi(bloques[i]);
            
            char pathBloqueLogico[512];
            snprintf(pathBloqueLogico, sizeof(pathBloqueLogico),
                     "%s/files/%s/%s/logical_blocks/%06d.dat",
                     configS->puntoMontaje, file, tag, i);

            // Contar referencias antes de eliminar
            int referencias = contarReferenciasBloque(pathBloqueLogico, queryID);
            
            if (referencias == 1) {
                // Única referencia → liberar bloque físico
                liberarBloque(bloqueFisico);
                log_info(logger, "##<%d> - Bloque Físico Liberado - Número de Bloque: <%d>",
                         queryID, bloqueFisico);
            }

            log_info(logger, "##<%d> - <%s:%s> Se eliminó el hard link del bloque lógico <%d> al bloque físico <%d>",
                     queryID, file, tag, i, bloqueFisico);
        }

        config_destroy(metadata);
    }

    // Eliminar directorio completo del Tag
    if (!eliminarDirectorioRecursivo(pathTag, queryID)) {
        log_error(logger, "##<%d> - Error al eliminar directorio <%s:%s>", 
                  queryID, file, tag);
        return false;
    }

    log_info(logger, "##<%d> - Tag Eliminado <%s:%s>", queryID, file, tag);
    return true;
}

bool eliminarDirectorioRecursivo(const char* path, int queryID) {
    DIR* dir = opendir(path);
    if (!dir) {
        log_error(logger, "##<%d> - Error al abrir directorio %s: %s", 
                  queryID, path, strerror(errno));
        return false;
    }

    struct dirent* entrada;
    bool exito = true;

    // Eliminar contenido del directorio
    while ((entrada = readdir(dir)) != NULL && exito) {
        // Ignorar . y ..
        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0) {
            continue;
        }

        char pathCompleto[1024];
        snprintf(pathCompleto, sizeof(pathCompleto), "%s/%s", path, entrada->d_name);

        struct stat st;
        if (stat(pathCompleto, &st) != 0) {
            log_error(logger, "##<%d> - Error al obtener info de %s: %s", 
                      queryID, pathCompleto, strerror(errno));
            exito = false;
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            // Es un subdirectorio → eliminar recursivamente
            if (!eliminarDirectorioRecursivo(pathCompleto, queryID)) {
                exito = false;
            }
        } else {
            // Es un archivo regular → eliminar
            if (unlink(pathCompleto) != 0) {
                log_error(logger, "##<%d> - Error al eliminar archivo %s: %s", 
                          queryID, pathCompleto, strerror(errno));
                exito = false;
            }
        }
    }

    closedir(dir);

    // Eliminar el directorio vacío
    if (exito && rmdir(path) != 0) {
        log_error(logger, "##<%d> - Error al eliminar directorio %s: %s", 
                  queryID, path, strerror(errno));
        return false;
    }

    return exito;
}