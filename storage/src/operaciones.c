#include "operaciones.h"

// READ:
char* lecturaBloque(char* file, char* tag, int numeroBloque){
    char* rutaCompleta = string_from_format(
        "%s/files/%s/%s/logical_blocks/%06d.dat",
        configS->puntoMontaje,file, tag, numeroBloque);

    FILE* bloque = fopen(rutaCompleta, "r");
    if (bloque == NULL) {
        log_debug(logger,"No Existe el bloque logico numero <%d> asociado al file:tag <%s:%s>\n", numeroBloque,file,tag);
        return NULL;
    }
    char* contenido = (char*)malloc((configSB->BLOCK_SIZE + 1) * sizeof(char));
    size_t bytesLeidos = fread(contenido, sizeof(char), configSB->BLOCK_SIZE, bloque);
    contenido[bytesLeidos] = '\0'; // Revisar si esto esta bien
    fclose(bloque);
    aplicarRetardoBloque();
    log_debug(logger,"Contenido del bloque <%d> asociado al file:tag <%s:%s>: %s",numeroBloque,file,tag,contenido);
    free(rutaCompleta);
    return contenido;
    
}


// TAG:
bool tagFile(char* fileOrigen, char* tagOrigen, char* fileDestino, char* tagDestino, int queryID){
    char* pathTagOrigen = string_from_format(
        "%s/files/%s/%s", 
        configS->puntoMontaje, fileOrigen, tagOrigen
    );
    
    char* pathFileDestino = string_from_format(
        "%s/files/%s", 
        configS->puntoMontaje, fileDestino
    );
    
    char* pathTagDestino = string_from_format(
        "%s/files/%s/%s", 
        configS->puntoMontaje, fileDestino, tagDestino
    );
    
    struct stat st;

    // Verificacion de existncia del TAG ORIGEN (No existe)
    if (stat(pathTagOrigen, &st) != 0) {
        log_debug(logger, "## <%d>- Error: Tag origen <%s:%s> no existe",queryID,fileOrigen, tagOrigen);
        free(pathTagOrigen);
        free(pathFileDestino);
        free(pathTagDestino);
        return false;
    }
    
    // Verificando de existencia del TAG DESTINO (Ya existe)
    if (stat(pathTagDestino, &st) == 0) {
        log_debug(logger, "## <%d>- Error: Tag destino <%s:%s> ya existe", queryID,fileDestino, tagDestino);
        free(pathTagOrigen);
        free(pathFileDestino);
        free(pathTagDestino);
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
    free(pathTagOrigen);
    free(pathFileDestino);
    free(pathTagDestino);
    return true;
}

bool copiarDirectorioRecursivo(const char* origen, const char* destino) {
    // Crear el directorio destino
    if (mkdir(destino, 0777) != 0 && errno != EEXIST) {
        log_error(logger, "Error al crear directorio destino: %s - %s", destino, strerror(errno));
        return false;
    }
    
    DIR* dir = opendir(origen);
    if (dir == NULL) {
        log_error(logger, "Error al abrir directorio origen: %s - %s", origen, strerror(errno));
        return false;
    }
    
    struct dirent* entrada;
    while ((entrada = readdir(dir)) != NULL) {
        // Ignorar . y ..
        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0) {
            continue;
        }
        
        char* pathOrigen = string_from_format("%s/%s", origen, entrada->d_name);
        char* pathDestino = string_from_format("%s/%s", destino, entrada->d_name);
        
        struct stat st;
        if (stat(pathOrigen, &st) != 0) {
            log_error(logger, "Error al obtener info de: %s", pathOrigen);
            free(pathOrigen);
            free(pathDestino);
            continue;
        }
        
        bool resultado = true;
        if (S_ISDIR(st.st_mode)) {
            // Es un directorio, copiar recursivamente
            if (!copiarDirectorioRecursivo(pathOrigen, pathDestino)) {
                resultado = false;
            }
        } else {
            // Es un archivo regular, copiarlo
            if (!copiarArchivo(pathOrigen, pathDestino)) {
                resultado = false;
            }
        }
        free(pathOrigen);
        free(pathDestino);
        
        if (!resultado) {
            closedir(dir);
            return false;
        }
    }
    
    closedir(dir);
    return true;
}

bool copiarArchivo(const char* origen, const char* destino) {
    struct stat st_origen;
    if (stat(origen, &st_origen) != 0) {
        log_error(logger, "Error al obtener info de archivo origen: %s", origen);
        return false;
    }
    
    // Si tiene más de un hardlink, intentar crear hardlink en destino
    if (st_origen.st_nlink > 1) {
        // Intentar crear hardlink
        if (link(origen, destino) == 0) {
            log_debug(logger, "Hardlink creado: %s -> %s", destino, origen);
            return true;
        }
        log_error(logger, "No se pudo crear hardlink, copiando contenido");
        exit(EXIT_FAILURE);
    }
    
    // Copiar contenido (código original)
    FILE* archivoOrigen = fopen(origen, "rb");
    if (archivoOrigen == NULL) {
        log_error(logger, "Error al abrir archivo origen: %s - %s", origen, strerror(errno));
        return false;
    }
    
    FILE* archivoDestino = fopen(destino, "wb");
    if (archivoDestino == NULL) {
        log_error(logger, "Error al crear archivo destino: %s - %s", destino, strerror(errno));
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
    chmod(destino, st_origen.st_mode);
    
    return true;
}

void aplicarRetardoBloque(){
    usleep(configS->retardoAccesoBloque*1000);
}

void aplicarRetardoOperacion(){
    usleep(configS->retardoOperacion*1000);
}


// WRITE:
bool escribirBloque(char* file, char* tag, int numeroBloqueLogico, char* contenido, int queryID) {
    char* pathTag = string_from_format("%s/files/%s/%s", configS->puntoMontaje, file, tag);
    char* pathBloqueLogico = string_from_format("%s/files/%s/%s/logical_blocks/%06d.dat", configS->puntoMontaje, file, tag, numeroBloqueLogico);
    struct stat st;
    
    // 1. Verificar si el bloque lógico no existe
    bool bloqueExiste = (stat(pathBloqueLogico, &st) == 0);
    if (!bloqueExiste){
        log_debug(logger,"## <%d> - Error bloque logico <%d> no existe",queryID,numeroBloqueLogico);
        free(pathBloqueLogico);
        return false;
    }
    
    // 2. Abrir el metadata que se encuentra en files/file/tag/metadata.config
    char* pathMetadata = string_from_format("%s/files/%s/%s/metadata.config", configS->puntoMontaje, file, tag);
    pthread_mutex_lock(&mutex_metadata);
    t_config* metadata = config_create(pathMetadata);
    if (metadata == NULL) {
        log_error(logger, "## <%d> - Error al abrir metadata de <%s:%s>", queryID, file, tag);
        pthread_mutex_unlock(&mutex_metadata);
        free(pathBloqueLogico);
        free(pathMetadata);
        exit(EXIT_FAILURE);
    }
    
    // 3. Verificar que el estado NO sea COMMITTED
    char* estado = config_get_string_value(metadata, "ESTADO");
    if (strcmp(estado, "COMMITTED") == 0) {
        log_debug(logger, "## <%d> - No se puede escribir en <%s:%s>: estado COMMITTED",queryID, file, tag);
        config_destroy(metadata);
        pthread_mutex_unlock(&mutex_metadata);
        free(pathBloqueLogico);
        free(pathMetadata);
        return false;
    }
    // 4. Calcular hash del contenido
    char* hash = crearHash(contenido);
    // 5. Verificar si el hash del contenido ya existe
    int bloqueConMismoHash = obtenerBloquePorHash(hash);
    if (bloqueConMismoHash >= 0) {
        // Ya existe un bloque físico con ese contenido, hacer hardlink
        log_debug(logger, "## <%d> - Bloque físico %d ya existe con hash %s, creando hardlink",queryID, bloqueConMismoHash, hash);
        char* pathBloqueFisicoExistente = string_from_format("%s/physical_blocks/block%04d.dat", configS->puntoMontaje, bloqueConMismoHash);
        
        if (link(pathBloqueFisicoExistente, pathBloqueLogico) != 0) {
            log_error(logger, "## <%d> - Error al crear hardlink: %s", queryID, strerror(errno));
            free(pathBloqueFisicoExistente);
            free(hash);
            config_destroy(metadata);
            pthread_mutex_unlock(&mutex_metadata);
            free(pathBloqueLogico);
            free(pathMetadata);
            exit(EXIT_FAILURE);
        }
        agregarBloqueMetaData(pathTag,numeroBloqueLogico,bloqueConMismoHash);
        free(pathBloqueFisicoExistente);
        log_info(logger, "##<%d> - <%s>:<%s> Hardlink creado por hash: bloque lógico <%d> -> bloque físico <%d>", queryID, file, tag, numeroBloqueLogico, bloqueConMismoHash);
        free(hash);
        config_destroy(metadata);
        pthread_mutex_unlock(&mutex_metadata);
        free(pathBloqueLogico);
        free(pathMetadata);
        return true;
    }
    
    // 6. No existe el hash - obtener el bloque físico actual
    int numeroBloqueFisicoActual = obtenerBloqueActual(file, tag, numeroBloqueLogico);
    if (numeroBloqueFisicoActual < 0) {
        log_error(logger, "## <%d> - Error al obtener bloque físico actual", queryID);
        free(hash);
        config_destroy(metadata);
        free(pathBloqueLogico);
        free(pathMetadata);
        exit(EXIT_FAILURE);
    }
    
    char* pathBloqueFisicoActual = string_from_format("%s/physical_blocks/block%04d.dat", configS->puntoMontaje, numeroBloqueFisicoActual);
    
    if (stat(pathBloqueFisicoActual, &st) != 0) {
        log_error(logger, "## <%d> - Error al obtener info del bloque físico %d", queryID, numeroBloqueFisicoActual);
        free(pathBloqueFisicoActual);
        free(hash);
        config_destroy(metadata);
        free(pathBloqueLogico);
        free(pathMetadata);
        exit(EXIT_FAILURE);
    }

    nlink_t numHardlinks = st.st_nlink;
    
    // 7. Si tiene más de 1 hardlink, necesitamos Copy-on-Write
    if (numHardlinks > 1) {
        log_debug(logger, "## <%d> - Bloque físico %d tiene %ld hardlinks, realizando Copy-on-Write", queryID, numeroBloqueFisicoActual, (long)numHardlinks);
        // Buscar bloque físico libre
        int numeroBloqueFisicoNuevo = bitmapReservarLibre();
        if (numeroBloqueFisicoNuevo < 0) {
            log_error(logger, "## <%d> - No hay bloques físicos disponibles", queryID);
            free(pathBloqueFisicoActual);
            free(hash);
            config_destroy(metadata);
            pthread_mutex_unlock(&mutex_metadata);
            free(pathBloqueLogico);
            free(pathMetadata);
            return false;
        }
        
        ocuparBloqueBit(numeroBloqueFisicoNuevo);
        char* pathNuevoBloqueFisico = string_from_format("%s/physical_blocks/%06d.dat", configS->puntoMontaje, numeroBloqueFisicoNuevo);
        if(link(pathNuevoBloqueFisico,pathBloqueLogico)!=0){
            exit(EXIT_FAILURE);
        }
        FILE* archivo = fopen(pathBloqueLogico, "wb");
        if (archivo == NULL) {
            log_error(logger, "## <%d> - Error al abrir bloque logico %d: %s", queryID, numeroBloqueFisicoNuevo, strerror(errno));
            free(pathBloqueLogico);
            free(pathBloqueFisicoActual);
            free(hash);
            config_destroy(metadata);
            free(pathMetadata);
            exit(EXIT_FAILURE);
        }
        
        fwrite(contenido, 1, strlen(contenido), archivo);
        fclose(archivo);
        agregarBloqueMetaData(pathTag, numeroBloqueLogico, numeroBloqueFisicoNuevo);
        escribirHash(hash, numeroBloqueFisicoNuevo);
        log_debug(logger, "##<%d> - <%s>:<%s> Copy-on-Write: bloque lógico <%d> -> nuevo bloque físico <%d>", queryID, file, tag, numeroBloqueLogico, numeroBloqueFisicoNuevo);
        
        free(pathNuevoBloqueFisico);
        
    } else {
        // 8. Tiene 1 solo hardlink, escribir directamente sobre el bloque lógico
        FILE* archivo = fopen(pathBloqueLogico, "wb");
        if (archivo == NULL) {
            log_error(logger, "## <%d> - Error al abrir bloque lógico para escritura: %s", queryID, strerror(errno));
            free(pathBloqueFisicoActual);
            free(hash);
            config_destroy(metadata);
            free(pathBloqueLogico);
            free(pathMetadata);
            exit(EXIT_FAILURE);
        }
        
        fwrite(contenido, 1, strlen(contenido), archivo);
        fclose(archivo);
        escribirHash(hash, numeroBloqueFisicoActual);
        
        log_debug(logger, "##<%d> - <%s>:<%s> Bloque lógico <%d> actualizado directamente (único hardlink)", queryID, file, tag, numeroBloqueLogico);
    }
    config_destroy(metadata);
    pthread_mutex_unlock(&mutex_metadata);
    aplicarRetardoBloque();
    free(pathBloqueFisicoActual);
    free(hash);
    free(pathBloqueLogico);
    free(pathMetadata);
    return true;
}


// TRUNCATE:
bool truncarArchivo(char* file, char* tag, int nuevoTamanio, int queryID) {
    // Validar que el tamaño sea múltiplo del tamaño de bloque
    if (nuevoTamanio % configSB->BLOCK_SIZE != 0) {
        log_error(logger, "##<%d> - Error: El tamaño <%d> no es múltiplo del tamaño de bloque <%d>", queryID, nuevoTamanio, configSB->BLOCK_SIZE);
        return false;
    }
    
    char* pathTag = string_from_format("%s/files/%s/%s", configS->puntoMontaje, file, tag);
    struct stat st;
    if (stat(pathTag, &st) != 0 || !S_ISDIR(st.st_mode)) {
        log_error(logger, "##<%d> - Error: File:Tag <%s:%s> no existe", queryID, file, tag);
        free(pathTag);
        return false;
    }
    free(pathTag);

    char* pathMetadata = string_from_format("%s/files/%s/%s/metadata.config", configS->puntoMontaje, file, tag);
    pthread_mutex_lock(&mutex_metadata);
    t_config* metadata = config_create(pathMetadata);
    free(pathMetadata);
    if (!metadata) {
        pthread_mutex_unlock(&mutex_metadata);
        log_error(logger, "##<%d> - Error al abrir metadata de <%s:%s>", queryID, file, tag);
        return false;
    }

    // Verificar que no esté COMMITED
    char* estado = config_get_string_value(metadata, "ESTADO");
    if (strcmp(estado, "COMMITED") == 0) {
        pthread_mutex_unlock(&mutex_metadata);
        log_error(logger, "##<%d> - Error: No se puede truncar <%s:%s> (COMMITED)", queryID, file, tag);
        config_destroy(metadata);
        return false;
    }

    // Obtener tamaño actual
    int tamanioActual = config_get_int_value(metadata, "TAMAÑO");
    int bloquesActuales = tamanioActual / configSB->BLOCK_SIZE;
    int bloquesNuevos = nuevoTamanio / configSB->BLOCK_SIZE;

    log_debug(logger, "##<%d> - Truncando <%s:%s>: %d bytes (%d bloques) -> %d bytes (%d bloques)", queryID, file, tag, tamanioActual, bloquesActuales, nuevoTamanio, bloquesNuevos);

    bool resultado = false;

    if (bloquesNuevos > bloquesActuales) {
        int bloquesNecesarios = bloquesNuevos - bloquesActuales;
        if (!hayEspacioDisponible(bloquesNecesarios, queryID)) {
            log_error(logger, "##<%d> - Error: Espacio insuficiente para truncar <%s:%s>", queryID, file, tag);
            config_destroy(metadata);
            return false;
        }
        resultado = aumentarTamanioArchivo(file, tag, bloquesActuales, bloquesNuevos, nuevoTamanio, metadata, queryID);
    } else if (bloquesNuevos < bloquesActuales) {
        resultado = reducirTamanioArchivo(file, tag, bloquesActuales, bloquesNuevos, nuevoTamanio, metadata, queryID);
    } else {
        config_set_value(metadata, "TAMAÑO", string_itoa(nuevoTamanio));
        config_save(metadata);
        resultado = true;
        
        log_info(logger, "##<%d> - File Truncado <%s:%s> - Tamaño: <%d>", queryID, file, tag, nuevoTamanio);
    }

    config_destroy(metadata);
    pthread_mutex_unlock(&mutex_metadata);
    return resultado;
}

bool aumentarTamanioArchivo(char* file, char* tag, int bloquesActuales, int bloquesNuevos, int nuevoTamanio, t_config* metadata, int queryID) {
    int bloquesAAgregar = bloquesNuevos - bloquesActuales;

    log_debug(logger, "##<%d> - Aumentando tamaño: agregar %d bloques lógicos", queryID, bloquesAAgregar);

    char* pathBloqueFisico0 = string_from_format("%s/physical_blocks/block0000.dat", configS->puntoMontaje);
    char* pathLogicalBlocks = string_from_format("%s/files/%s/%s/logical_blocks", configS->puntoMontaje, file, tag);

    // Crear hardlinks apuntando al bloque 0
    for (int i = bloquesActuales; i < bloquesNuevos; i++) {
        char* pathBloqueLogico = string_from_format("%s/%06d.dat", pathLogicalBlocks, i);

        if (link(pathBloqueFisico0, pathBloqueLogico) != 0) {
            log_error(logger, "##<%d> - Error al crear hardlink para bloque lógico %d: %s", queryID, i, strerror(errno));
            free(pathBloqueLogico);
            free(pathLogicalBlocks);
            free(pathBloqueFisico0);
            return false;
        }

        log_info(logger, "##<%d> - <%s:%s> Se agregó el hard link del bloque lógico <%d> al bloque físico <0>", queryID, file, tag, i);

        char* pathTag = string_from_format("%s/files/%s/%s", configS->puntoMontaje, file, tag);
        agregarBloqueMetaData(pathTag, i, 0);
        free(pathTag);
        // agregarBloqueMetaData(string_from_format("%s/files/%s/%s", configS->puntoMontaje, file, tag), 0,0);
        
        free(pathBloqueLogico);
    }

    actualizarMetadataTruncate(metadata, nuevoTamanio, bloquesNuevos);

    log_info(logger, "##<%d> - File Truncado <%s:%s> - Tamaño: <%d>", queryID, file, tag, nuevoTamanio);

    free(pathLogicalBlocks);
    free(pathBloqueFisico0);
    return true;
}

int contarReferenciasBloque(char* pathBloqueLogico, int queryID) {
    struct stat st;
    
    if (stat(pathBloqueLogico, &st) != 0) {
        log_error(logger, "##<%d> - Error al obtener info del bloque lógico: %s", queryID, strerror(errno));
        return -1;
    }

    return st.st_nlink;
}

bool reducirTamanioArchivo(char* file, char* tag, int bloquesActuales, int bloquesNuevos, int nuevoTamanio, t_config* metadata, int queryID) {
    
    int bloquesAEliminar = bloquesActuales - bloquesNuevos;

    log_debug(logger, "##<%d> - Reduciendo tamaño: eliminar %d bloques lógicos (desde el final)", queryID, bloquesAEliminar);

    char** bloques = config_get_array_value(metadata, "BLOQUES");
    if (!bloques) {
        log_error(logger, "##<%d> - Error al obtener bloques de metadata", queryID);
        return false;
    }

    // Eliminar bloques desde el final
    for (int i = bloquesActuales - 1; i >= bloquesNuevos; i--) {
        int bloqueFisico = atoi(bloques[i]);

        char* pathBloqueLogico = string_from_format("%s/files/%s/%s/logical_blocks/%06d.dat", configS->puntoMontaje, file, tag, i);

        int referencias = contarReferenciasBloque(pathBloqueLogico, queryID);
        if (referencias < 0) {
            log_error(logger, "##<%d> - Error al obtener referencias del bloque lógico %d", queryID, i);
            free(pathBloqueLogico);
            string_array_destroy(bloques);
            return false;
        }

        if (unlink(pathBloqueLogico) != 0) {
            log_error(logger, "##<%d> - Error al eliminar bloque lógico %d: %s", queryID, i, strerror(errno));
            free(pathBloqueLogico);
            string_array_destroy(bloques);
            return false;
        }

        log_info(logger, "##<%d> - <%s:%s> Se eliminó el hard link del bloque lógico <%d> al bloque físico <%d>", queryID, file, tag, i, bloqueFisico);

        if (referencias == 1) {
            liberarBloque(bloqueFisico, queryID);
        }
        
        free(pathBloqueLogico);
    }

    string_array_destroy(bloques);

    actualizarMetadataTruncate(metadata, nuevoTamanio, bloquesNuevos);

    log_info(logger, "##<%d> - File Truncado <%s:%s> - Tamaño: <%d>", queryID, file, tag, nuevoTamanio);

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
    char* tamanioStr = string_itoa(nuevoTamanio);
    config_set_value(metadata, "TAMAÑO", tamanioStr);
    free(tamanioStr);

    char** bloquesActuales = config_get_array_value(metadata, "BLOQUES");

    char* nuevosBloquesStr = string_new();
    string_append(&nuevosBloquesStr, "[");

    for (int i = 0; i < cantidadBloques; i++) {
        char* bloqueStr = (bloquesActuales && bloquesActuales[i]) ? string_duplicate(bloquesActuales[i]) : string_itoa(0);
        
        string_append(&nuevosBloquesStr, bloqueStr);
        
        if (i < cantidadBloques - 1) {
            string_append(&nuevosBloquesStr, ",");
        }
        
        free(bloqueStr);
    }

    string_append(&nuevosBloquesStr, "]");

    config_set_value(metadata, "BLOQUES", nuevosBloquesStr);
    config_save(metadata);

    if (bloquesActuales) {
        string_array_destroy(bloquesActuales);
    }
    free(nuevosBloquesStr);
}


// COMMIT:
void hacerCommited(char* file, char* tag,int queryID) {
    char* pathMetadata = string_from_format("%s/files/%s/%s/metadata.config", configS->puntoMontaje, file, tag);
    pthread_mutex_lock(&mutex_metadata);
    if (esCommited(file, tag, pathMetadata)) {
        pthread_mutex_unlock(&mutex_metadata);
        free(pathMetadata);
        return;
    }
    pthread_mutex_unlock(&mutex_metadata);

    recorrerBloquesLogicos(file, tag, queryID);

    pthread_mutex_lock(&mutex_metadata);
    cambiarEstadoMetaData(pathMetadata, "COMMITED");
    pthread_mutex_unlock(&mutex_metadata); 
    
    free(pathMetadata);
    return;
}

bool esCommited(char* file, char* tag, char*pathMetaData) {

    t_config* config = config_create(pathMetaData);
    if (config == NULL) {
        log_error(logger, "## - Error al abrir metadata de <%s:%s>", file, tag);
        exit(EXIT_FAILURE);
    }
    
    char* estadoActual = config_get_string_value(config, "ESTADO");
    if (strcmp(estadoActual, "COMMITED") == 0) {
        log_debug(logger, "## - File:Tag <%s:%s> ya está confirmado (COMMITED)",file, tag);
        config_destroy(config);
        return true;  
    }
    config_destroy(config);
    log_error(logger,"Esto no deberia pasar");
    return false;
}

void recorrerBloquesLogicos(char* file, char* tag, int queryID) {
    char* pathLogicalBlocks = string_from_format("%s/files/%s/%s/logical_blocks", configS->puntoMontaje, file, tag);
    
    DIR* dir = opendir(pathLogicalBlocks);
    if (dir == NULL) {
        log_error(logger, "Error al abrir directorio logical_blocks de <%s:%s>: %s", file, tag, strerror(errno));
        free(pathLogicalBlocks);
        exit(EXIT_FAILURE);
    }
    struct dirent* entrada;
    while ((entrada = readdir(dir)) != NULL) {
        // Ignorar . y ..
        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0) {
            continue;
        }
        if (strstr(entrada->d_name, ".dat") == NULL) {
            continue;
        }
        int numeroBloqueLogico = atoi(entrada->d_name);
        char* pathBloqueLogico = string_from_format("%s/%s",pathLogicalBlocks, entrada->d_name);
        procesarBloqueLogico(pathBloqueLogico, file, tag, numeroBloqueLogico, queryID);
        free(pathBloqueLogico);
    }
    closedir(dir);
    free(pathLogicalBlocks);
}

int obtenerBloqueFisico(char* file, char* tag, int numeroBloqueLogico) {
    char* pathMetadata = string_from_format("%s/files/%s/%s/metadata.config", configS->puntoMontaje, file, tag);
    pthread_mutex_lock(&mutex_metadata);
    t_config* config = config_create(pathMetadata);
    if (config == NULL) {
        log_error(logger, "No se pudo abrir el archivo de metadata: %s", pathMetadata);
        free(pathMetadata);
        exit(EXIT_FAILURE);
    }
    
    int bloqueFisico = -1;
    
    if (config_has_property(config, "BLOQUES")) {
        char** bloques = config_get_array_value(config, "BLOQUES");
        
        // Verificar que el número de bloque lógico esté en rango
        int cantidadBloques = 0;
        while (bloques[cantidadBloques] != NULL) {
            cantidadBloques++;
        }
        
        if (numeroBloqueLogico < cantidadBloques) {
            bloqueFisico = atoi(bloques[numeroBloqueLogico]);
        } else {
            log_error(logger, "Bloque lógico %d fuera de rango (total: %d)", numeroBloqueLogico, cantidadBloques);
            exit(EXIT_FAILURE);
        }
        
        // Liberar el array
        for (int i = 0; bloques[i] != NULL; i++) { free(bloques[i]); }
        free(bloques);
    }
    
    config_destroy(config);
    pthread_mutex_unlock(&mutex_metadata);
    free(pathMetadata);
    return bloqueFisico;
}

void procesarBloqueLogico(char* pathBloqueLogico, char* file, char* tag, int numeroBloqueLogico, int queryID){
    char* hashContenido = calcularHashArchivo(pathBloqueLogico);    
    log_debug(logger, "Hash calculado para bloque lógico %d: %s",numeroBloqueLogico, hashContenido);
    
    /// Caso de que el hash exista
    if (existeHash(hashContenido)) {
        // El contenido ya existe, obtener el bloque físico existente
        int anteriorBloqueFisico = obtenerBloqueFisico(file, tag, numeroBloqueLogico);
        int nuevoBloqueFisico = obtenerBloquePorHash(hashContenido);
        if (nuevoBloqueFisico != -1) {
            reapuntarBloqueLogico(file,tag,numeroBloqueLogico,anteriorBloqueFisico,nuevoBloqueFisico,queryID);
            actualizarMetadataBloques(file, tag , numeroBloqueLogico, nuevoBloqueFisico, anteriorBloqueFisico, queryID);
            
        }
    } else {
        // Caso de que el hash no exista
        int bloqueActual = obtenerBloqueActual(file, tag, numeroBloqueLogico);
        log_debug(logger, "Nuevo contenido: agregando hash %s -> bloque físico %d",hashContenido, bloqueActual);
        escribirHash(hashContenido, bloqueActual);
        
    }
    free(hashContenido);
}

void reapuntarBloqueLogico(char* file, char *tag, int numeroBloqueLogico,int previoBloqueFisico ,int nuevoBloqueFisico, int queryID) {
    char* pathBloqueLogico = string_from_format("%s/files/%s/%s/logical_blocks/block%04d.dat",configS->puntoMontaje,file,tag, numeroBloqueLogico);
    // Eliminamos Hardlink
    if (unlink(pathBloqueLogico) != 0) {
        log_error(logger, "Error al eliminar hardlink: %s - %s", pathBloqueLogico, strerror(errno));
        exit(EXIT_FAILURE);
    }
    log_info(logger,"##<%d> - <%s>:<%s> Se eliminó el hard link del bloque lógico <%d> al bloque físico <%d>",queryID,file,tag,numeroBloqueLogico,previoBloqueFisico);
    liberarBloque(previoBloqueFisico, queryID);
    char* pathBloqueFisico = string_from_format("%s/physical_blocks/block%04d.dat",configS->puntoMontaje, nuevoBloqueFisico);
    
    // Creamos nuevo Hardlink
    if (link(pathBloqueFisico, pathBloqueLogico) != 0) {
        log_error(logger, "Error al crear nuevo hardlink: %s -> %s - %s", pathBloqueLogico, pathBloqueFisico, strerror(errno));
        free(pathBloqueFisico);
        exit(EXIT_FAILURE);
    }
    log_info(logger,"##<%d> - <%s>:<%s> Se agregó el hard link del bloque lógico <%d> al bloque físico <%d>",queryID,file,tag,numeroBloqueLogico,nuevoBloqueFisico);
    free(pathBloqueFisico);
}

int obtenerBloqueActual(char* file, char* tag, int numeroBloqueLogico) {
    char* pathMetadata = string_from_format("%s/files/%s/%s/metadata.config",configS->puntoMontaje, file, tag);
    pthread_mutex_lock(&mutex_metadata);
    t_config* config = config_create(pathMetadata);
    if (config == NULL) {
        pthread_mutex_unlock(&mutex_metadata);
        log_error(logger, "Error al abrir metadata de <%s:%s>", file, tag);
        free(pathMetadata);
        exit(EXIT_FAILURE);
    }
    
    char** arrayBloques = config_get_array_value(config, "BLOQUES");
    
    if (arrayBloques == NULL) {
        log_error(logger, "No se pudo leer BLOQUES del metadata");
        config_destroy(config);
        free(pathMetadata);
        exit(EXIT_FAILURE);
    }
    
    // Obtener el bloque físico en la posición numeroBloqueLogico
    int bloqueFisico = atoi(arrayBloques[numeroBloqueLogico]);
    
    for (int i = 0; arrayBloques[i] != NULL; i++) {
        free(arrayBloques[i]);
    }

    free(arrayBloques);
    config_destroy(config);
    pthread_mutex_unlock(&mutex_metadata);
    free(pathMetadata);
    return bloqueFisico;
}

void actualizarMetadataBloques(char* file, char* tag, int numeroBloqueLogico, int numeroNuevoBloqueFisico, int anteriorBloqueFisico, int queryID) {
    char* pathMetadata = string_from_format("%s/files/%s/%s/metadata.config",configS->puntoMontaje, file, tag);
    pthread_mutex_lock(&mutex_metadata);
    t_config* config = config_create(pathMetadata);
    if (config == NULL) {
        log_error(logger, "Error al abrir metadata de <%s:%s>", file, tag);
        free(pathMetadata);
        exit(EXIT_FAILURE);
    }
    char** arrayBloques = config_get_array_value(config, "BLOQUES");
    if (arrayBloques == NULL) {
        log_error(logger, "No se pudo leer BLOQUES del metadata");
        config_destroy(config);
        free(pathMetadata);
        exit(EXIT_FAILURE);
    }
    
    free(arrayBloques[numeroBloqueLogico]);
    arrayBloques[numeroBloqueLogico] = string_itoa(numeroNuevoBloqueFisico);
    
    char* nuevoArrayStr = construirStringArray(arrayBloques);
    config_set_value(config, "BLOQUES", nuevoArrayStr);
    config_save(config);
    
    for (int i = 0; arrayBloques[i] != NULL; i++) {
        free(arrayBloques[i]);
    }

    free(arrayBloques);
    free(nuevoArrayStr);
    free(pathMetadata);

    config_destroy(config);
    pthread_mutex_unlock(&mutex_metadata);
    log_info(logger,"##<%d> - <%s>:<%s> Bloque Lógico <%d> se reasigna de <%d> a <%d>", queryID, file, tag, numeroBloqueLogico, anteriorBloqueFisico, numeroNuevoBloqueFisico);

}

char* construirStringArray(char** array) {
    if (array == NULL || array[0] == NULL) {
        return string_duplicate("[]");
    }
    
    char* resultado = string_duplicate("[");
    
    for (int i = 0; array[i] != NULL; i++) {
        string_append(&resultado, array[i]);
        
        if (array[i + 1] != NULL) {
            string_append(&resultado, ",");
        }
    }
    string_append(&resultado, "]");
    return resultado;
}


// DELETE:
bool eliminarTag(char* file, char* tag, int queryID) {
    // 1. Construir el path al tag
    char* pathFileTag = string_from_format("%s/files/%s/%s", configS->puntoMontaje, file, tag);
    
    // 2. Verificar si existe
    struct stat st;
    if (stat(pathFileTag, &st) != 0) {
        log_debug(logger, "## <%d> - Error: File:Tag <%s:%s> no existe",queryID, file, tag);
        free(pathFileTag);
        return false;
    }

    if (strcmp(file, "initial_file") == 0) { // && strcmp(tag, "BASE") == 0
        log_error(logger, "## <%d> - Error: No se puede eliminar initial_file:BASE (protegido por el sistema)", queryID);
        return false;
    }
    
    // Eliminar Bloques Logicos
    eliminarBloquesLogicos(file, tag, queryID);
    
    // Eliminar path de Bloques Logicos
    char* pathLogicalBlocks = string_from_format("%s/logical_blocks",pathFileTag);
    
    if (rmdir(pathLogicalBlocks) != 0) {
        log_error(logger, "## <%d> - Error al eliminar directorio logical_blocks: %s", queryID, strerror(errno));
        free(pathLogicalBlocks);
        exit(EXIT_FAILURE);
    }
    
    // Eliminar metadata.config
    char* pathMetadata = string_from_format("%s/metadata.config", pathFileTag);
    
    if (unlink(pathMetadata) != 0) {
        log_error(logger, "## <%d> - Error al eliminar metadata.config: %s", queryID, strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    // Eliminar Directorio Tag
    if (rmdir(pathFileTag) != 0) {
        log_error(logger, "## <%d> - Error al eliminar directorio del tag: %s", queryID, strerror(errno));
        free(pathFileTag);
        free(pathMetadata);
        exit(EXIT_FAILURE);
    }
    
    free(pathMetadata);
    free(pathFileTag);
    free(pathLogicalBlocks);

    return true;
}

void eliminarBloqueLogico(char* pathBloqueLogico, char* file, char* tag, int numeroBloqueLogico, int queryID) {
    int numeroBloqueFisico = obtenerBloqueActual(file, tag, numeroBloqueLogico);
    if (numeroBloqueFisico < 0) {
        log_error(logger, "## <%d> - Error: no se pudo obtener bloque físico para bloque lógico %d de %s:%s", queryID, numeroBloqueLogico, file, tag);
        exit(EXIT_FAILURE);
    }
    
    char* pathBloqueFisico = string_from_format("%s/physical_blocks/block%04d.dat", configS->puntoMontaje, numeroBloqueFisico);
    
    struct stat st;
    if (stat(pathBloqueFisico, &st) != 0) {
        log_error(logger, "## <%d> - Error al obtener info del bloque físico %d: %s", queryID, numeroBloqueFisico, strerror(errno));
        free(pathBloqueFisico);
        exit(EXIT_FAILURE);
    }
    
    nlink_t numHardlinks = st.st_nlink;
    
    if (unlink(pathBloqueLogico) != 0) {
        log_error(logger, "## <%d> - Error al eliminar bloque lógico %s: %s", queryID, pathBloqueLogico, strerror(errno));
        free(pathBloqueFisico);
        exit(EXIT_FAILURE);
    }
    
    log_info(logger, "##<%d> - <%s>:<%s> Se eliminó el hard link del bloque lógico <%d> al bloque físico <%d>", queryID, file, tag, numeroBloqueLogico, numeroBloqueFisico);
    
    if (numHardlinks == 1) {
        liberarBloque(numeroBloqueFisico, queryID);
    } else {
        log_debug(logger, "## <%d> - Bloque físico %d aún tiene %ld hardlinks, no se libera", queryID, numeroBloqueFisico, (long)(numHardlinks - 1));
    }
    
    free(pathBloqueFisico);
}
    
void eliminarBloquesLogicos(char* file, char* tag, int queryID) {
    char* pathLogicalBlocks = string_from_format( "%s/files/%s/%s/logical_blocks", configS->puntoMontaje, file, tag);
    
    DIR* dir = opendir(pathLogicalBlocks);
    if (dir == NULL) {
        log_error(logger, "## <%d> - Error al abrir logical_blocks: %s", queryID, strerror(errno));
        free(pathLogicalBlocks);
        exit(EXIT_FAILURE);
    }
    
    struct dirent* entrada;
    while ((entrada = readdir(dir)) != NULL) {
        // Ignorar . y ..
        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0) {
            continue;
        }
        
        // Solo procesar archivos .dat
        if (strstr(entrada->d_name, ".dat") == NULL) {
            continue;
        }
        
        // Extraer el número de bloque lógico del nombre
        // Formato esperado: "000000.dat", "000001.dat", etc.
        int numeroBloqueLogico = atoi(entrada->d_name);
        
        // Construir path completo del bloque lógico
        char* pathBloqueLogico = string_from_format( "%s/%s", pathLogicalBlocks, entrada->d_name);
        
        // Eliminar el bloque lógico (ahora con el parámetro correcto)
        eliminarBloqueLogico(pathBloqueLogico, file, tag, numeroBloqueLogico, queryID);
        
        free(pathBloqueLogico);
    }
    
    closedir(dir);
    free(pathLogicalBlocks);
}