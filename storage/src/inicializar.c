#include "inicializar.h"

char* pathBloquesFisicos;
char* pathFiles;
void inicializarArchivo(const char *rutaBase, const char *nombre, const char *extension,char* modoApertura) {
    char archivoPath[512];
    snprintf(archivoPath, sizeof(archivoPath), "%s/%s.%s", rutaBase, nombre, extension);

    FILE* archivo = fopen(archivoPath, modoApertura);
    if (archivo == NULL) {
        fprintf(stderr, "No se pudo crear/abrir %s: %s\n", archivoPath, strerror(errno));
        exit(EXIT_FAILURE);
    }
    fclose(archivo);
    if (modoApertura[0] == 'w' || modoApertura[0] == 'a') {
        if (chmod(archivoPath, 0777) != 0) {
            log_error(logger, "No se pudieron asignar permisos a %s: \n", archivoPath);
        }
    }
}


// void inicializarBlocksHashIndex(char* path) {
//     char archivoPath[512];
//     snprintf(archivoPath, sizeof(archivoPath), "%s/blocks_hash_index.config", path);

//     // Si ya existe, no hacer nada
//     if (access(archivoPath, F_OK) == 0) {
//         log_debug(logger, "Archivo blocks_hash_index.config ya existe, no se sobrescribe");
//         return;
//     }

//     // Crear archivo vacío
//     inicializarArchivo(path, "blocks_hash_index", "config", "w");
//     log_debug(logger, "Archivo blocks_hash_index.config inicializado correctamente");
// }

char* inicializarDirectorio(char* pathBase, char* nombreDirectorio){ 
    char dirPath[512];
    snprintf(dirPath, sizeof(dirPath), "%s/%s", pathBase, nombreDirectorio);

    // mkdir devuelve 0 si creó el directorio, -1 si hubo error
    if (mkdir(dirPath, 0777) == -1) {
        if (errno != EEXIST) { // si el directorio ya existe, no es error
            log_error(logger, "No se pudo crear el directorio %s: %s", nombreDirectorio, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    return strdup(dirPath);
}

void inicializarBloquesFisicos(char* pathPhysicalBlocks) {
    if (configS->freshStart) {
        int cantBloques = configSB->FS_SIZE / configSB->BLOCK_SIZE;

        for (int i = 0; i < cantBloques; i++) {
            char nombreBloque[32];
            snprintf(nombreBloque, sizeof(nombreBloque), "block%04d", i);

            inicializarArchivo(pathPhysicalBlocks, nombreBloque, "dat", "w+");
        }
    }
}

void inicializarMutex(void){
    pthread_mutex_init(&mutex_hash_block,NULL);
    pthread_mutex_init(&mutex_numero_bloque,NULL);
    pthread_mutex_init(&mutex_cantidad_workers,NULL);
}

char* crearHash(const char* contenido) {   
    char* hash = crypto_md5((void*)contenido, configSB->BLOCK_SIZE);
    
    if (!hash) {
        log_error(logger, "Error al calcular hash MD5");
        exit(EXIT_FAILURE);
    }

    return hash;
}

void inicializarBloqueCero(char* pathPhysicalBlocks) {
    char pathBloque[512];
    snprintf(pathBloque, sizeof(pathBloque), "%s/block0000.dat", pathPhysicalBlocks);

    char* contenido = malloc(configSB->BLOCK_SIZE);
    if (contenido == NULL) {
        log_error(logger, "Error al asignar memoria para contenido del bloque 0");
        exit(EXIT_FAILURE);
    }

    memset(contenido, '0', configSB->BLOCK_SIZE);

    char* hash = crearHash(contenido);
    if (!hash) {
        log_error(logger, "Error al crear hash del bloque 0");
        free(contenido);
        exit(EXIT_FAILURE);
    }

    escribirHash(hash, 0);
    free(hash);

    FILE* archivo = fopen(pathBloque, "w");
    if (archivo == NULL) {
        log_error(logger, "Error al abrir block0000.dat para inicialización");
        free(contenido);
        exit(EXIT_FAILURE);
    }
    
    fwrite(contenido, 1, configSB->BLOCK_SIZE, archivo);
    fclose(archivo);
    free(contenido);
    
    if (ocuparBloqueBit(0) != 0) {
        log_error(logger, "Error al marcar bloque 0 como ocupado en bitmap");
        exit(EXIT_FAILURE);
    }
}

void levantarFileSystem(){
    if (configS->freshStart){
        pathBloquesFisicos = inicializarDirectorio(configS->puntoMontaje,"physical_blocks");
        char* pathBitMap = string_from_format("%s/bitmap.bin", configS->puntoMontaje);
        inicializarBitmap(pathBitMap);
        inicializarBlocksHashIndex(configS->puntoMontaje);
        inicializarBloquesFisicos(pathBloquesFisicos);
        inicializarBloqueCero(pathBloquesFisicos);

        pathFiles = inicializarDirectorio(configS->puntoMontaje, "files");
        crearFile("initial_file","BASE");

        char* pathTagBase = string_from_format("%s/initial_file/BASE", pathFiles);  
        agregarBloqueMetaData(pathTagBase,0);
        agregarBloquesLogicos(pathTagBase,configSB->BLOCK_SIZE);
        cambiarEstadoMetaData(pathTagBase, "COMMITTED");
        free(pathTagBase);


    }
    else{
    }
}

void crearFile(char* nombreFile, char* nombreTag){
    char * path = inicializarDirectorio(pathFiles,nombreFile);
    crearTag(path,nombreTag);
    free(path);
}

void crearTag(char* pathFile,char* nombreTag){
    char *pathTag =  inicializarDirectorio(pathFile,nombreTag);
    char* pathLogicalBlocks = inicializarDirectorio(pathTag,"logical_blocks");
    crearMetaData(pathTag);

    free(pathTag);
    free(pathLogicalBlocks);
}

void crearMetaData(char*pathTag){
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
// void agregarBloqueMetaData(char* pathTag, int nuevoBloque) {
//     char* pathMetadata = string_from_format("%s/metadata.config", pathTag);
//     t_config* config = config_create(pathMetadata);

//     char** bloquesActuales = config_get_array_value(config, "BLOQUES");

//     char* nuevoValor = string_new();
//     string_append(&nuevoValor, "[");

//     // concatenar bloques existentes
//     if (bloquesActuales != NULL) {
//         for (int i = 0; bloquesActuales[i] != NULL; i++) {
//             string_append(&nuevoValor, bloquesActuales[i]);
//             //string_append(&nuevoValor, ","); Supuestamente lo agrega
//         }
//     }

//     char* numStr = string_from_format("%d", nuevoBloque);
//     string_append(&nuevoValor, numStr);
//     string_append(&nuevoValor, "]");

//     config_set_value(config, "BLOQUES", nuevoValor);
//     config_save(config);

//     free(numStr);
//     free(nuevoValor);
//     free(pathMetadata);
//     config_destroy(config);

// }
void agregarBloqueMetaData(char* pathTag, int nuevoBloque) {
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
        // Usar string_get_string_as_array para parsear el string "[1,2,3]"
        bloquesActuales = string_get_string_as_array(bloquesString);
        
        // Contar bloques existentes
        if (bloquesActuales != NULL) {
            while (bloquesActuales[cantidadBloques] != NULL) {
                cantidadBloques++;
            }
        }
    }
    
    // Crear array para incluir el nuevo bloque
    int* bloques = malloc(sizeof(int) * (cantidadBloques + 1));
    
    // Copiar bloques existentes al array de enteros
    for (int i = 0; i < cantidadBloques; i++) {
        bloques[i] = atoi(bloquesActuales[i]);
    }
    
    // Agregar el nuevo bloque
    bloques[cantidadBloques] = nuevoBloque;
    cantidadBloques++;
    
    // Ordenar de menor a mayor (bubble sort simple)
    for (int i = 0; i < cantidadBloques - 1; i++) {
        for (int j = 0; j < cantidadBloques - i - 1; j++) {
            if (bloques[j] > bloques[j + 1]) {
                int temp = bloques[j];
                bloques[j] = bloques[j + 1];
                bloques[j + 1] = temp;
            }
        }
    }
    
    // Construir el string con los bloques ordenados
    char* nuevoValor = string_new();
    string_append(&nuevoValor, "[");
    for (int i = 0; i < cantidadBloques; i++) {
        char* numStr = string_from_format("%d", bloques[i]);
        string_append(&nuevoValor, numStr);
        if (i < cantidadBloques - 1) {
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
    
    log_debug(logger, "Bloque %d agregado a metadata en %s", nuevoBloque, pathTag);
}
// void agregarBloqueMetaData(char* pathTag, int nuevoBloque) {
//     char* pathMetadata = string_from_format("%s/metadata.config", pathTag);
//     t_config* config = config_create(pathMetadata);
    
//     if (config == NULL) {
//         log_error(logger, "Error al abrir metadata en: %s", pathMetadata);
//         free(pathMetadata);
//         exit(EXIT_FAILURE);
//     }
    
//     char** bloquesActuales = config_get_array_value(config, "BLOQUES");
    
//     // Contar bloques existentes
//     int cantidadBloques = 0;
//     if (bloquesActuales != NULL) {
//         while (bloquesActuales[cantidadBloques] != NULL) {
//             cantidadBloques++;
//         }
//     }
    
//     int* bloques = malloc(sizeof(int) * (cantidadBloques + 1));
    
//     // Copiar bloques existentes al array de enteros
//     for (int i = 0; i < cantidadBloques; i++) {
//         bloques[i] = atoi(bloquesActuales[i]);
//     }
    
//     // Agregar el nuevo bloque
//     bloques[cantidadBloques] = nuevoBloque;
//     cantidadBloques++;
    
//     // Ordenar de menor a mayor (bubble sort simple)
//     for (int i = 0; i < cantidadBloques - 1; i++) {
//         for (int j = 0; j < cantidadBloques - i - 1; j++) {
//             if (bloques[j] > bloques[j + 1]) {
//                 int temp = bloques[j];
//                 bloques[j] = bloques[j + 1];
//                 bloques[j + 1] = temp;
//             }
//         }
//     }
    
//     // Construir el string con los bloques ordenados
//     char* nuevoValor = string_new();
//     string_append(&nuevoValor, "[");
    
//     for (int i = 0; i < cantidadBloques; i++) {
//         char* numStr = string_from_format("%d", bloques[i]);
//         string_append(&nuevoValor, numStr);
        
//         if (i < cantidadBloques - 1) {  // Agregar coma si NO es el último
//             string_append(&nuevoValor, ",");
//         }
//         free(numStr);
//     }
    
//     string_append(&nuevoValor, "]");
    
//     // Guardar y limpiar
//     config_set_value(config, "BLOQUES", nuevoValor);
//     config_save(config);
    
//     free(bloques);
//     free(nuevoValor);
//     free(pathMetadata);
//     config_destroy(config);
// }

// void agregarBloquesLogicos(char* pathTag, int tamanioArchivo) {
//     // calculo de cantidad de bloques logicos
//     int cantidadBloques = tamanioArchivo / configSB->BLOCK_SIZE;
//     // creamos path al bloque fisico numero 0
//     char *pathBloqueFisico0 = string_from_format("%s/block0000.dat", pathFiles);
//     // Crear cada bloque logico como hardlink
//     for (int i = 0; i < cantidadBloques; i++) {
//         char *pathBloqueLogicos = string_from_format("%s/%s/logical_blocks/%06d.dat",pathTag,i);

//         if (link(pathBloqueFisico0, pathBloqueLogicos) != 0) {
//             log_error(logger, "Error al crear hardlink para bloque %d: %s", i, strerror(errno));
//             exit(EXIT_FAILURE);
//         }
//     }
//     log_debug(logger, "Agregando <%d> bloques logicos para el file:tag <%s>", cantidadBloques,pathTag);
// }



void agregarBloquesLogicos(char* pathTag, int tamanioArchivo) {

    int cantidadBloquesNecesarios = tamanioArchivo / configSB->BLOCK_SIZE;
    char *pathBloqueFisico0 = string_from_format("%s/block0000.dat", pathBloquesFisicos);    
    char *pathLogicalBlocks = string_from_format("%s/logical_blocks", pathTag);
    int bloquesExistentes = 0;

    DIR* dir = opendir(pathLogicalBlocks);
    if (dir != NULL) {
        struct dirent* entrada;
        while ((entrada = readdir(dir)) != NULL) {
            if (strstr(entrada->d_name, ".dat") != NULL) {
                bloquesExistentes++;
            }
        }
        closedir(dir);
    }
    
    //log_debug(logger, "Bloques existentes: %d, Necesarios: %d", bloquesExistentes, cantidadBloquesNecesarios);
    
    for (int i = bloquesExistentes; i < cantidadBloquesNecesarios; i++) {
        char *pathBloqueLogico = string_from_format("%s/%06d.dat", pathLogicalBlocks, i);
        if (link(pathBloqueFisico0, pathBloqueLogico) != 0) {
            log_error(logger, "Error al crear hardlink para bloque %d: %s", i, strerror(errno));
            free(pathBloqueLogico);
            free(pathLogicalBlocks);
            free(pathBloqueFisico0);
            exit(EXIT_FAILURE);
        }
        log_debug(logger, "Bloque lógico %d creado", i);
        free(pathBloqueLogico);
    }
    int bloquesAgregados = cantidadBloquesNecesarios - bloquesExistentes;
    log_debug(logger, "Agregados %d bloques lógicos nuevos para tag <%s>", bloquesAgregados, pathTag);
    free(pathLogicalBlocks);
    free(pathBloqueFisico0);
}