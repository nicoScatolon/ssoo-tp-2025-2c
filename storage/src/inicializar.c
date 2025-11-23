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
        free(pathBitMap);

        inicializarBlocksHashIndex(configS->puntoMontaje);
        inicializarBloquesFisicos(pathBloquesFisicos);
        inicializarBloqueCero(pathBloquesFisicos);

        pathFiles = inicializarDirectorio(configS->puntoMontaje, "files");
        crearFile("initial_file","BASE");

        char* pathTagBase = string_from_format("%s/initial_file/BASE", pathFiles);  
        agregarBloqueMetaData(pathTagBase,0,0);
        agregarBloquesLogicos(pathTagBase,configSB->BLOCK_SIZE);
        cambiarEstadoMetaData(pathTagBase, "COMMITED");
        free(pathTagBase);


    }
    else{
    }
}

bool crearFile(char* nombreFile, char* nombreTag){
    char* pathFile = string_from_format("%s/%s", pathFiles, nombreFile);
    struct stat st;
    if (stat(pathFile, &st) != 0) {
        if (mkdir(pathFile, 0777) != 0) {
            log_error(logger, "Error al crear directorio del File ya existe <%s>: %s", nombreFile, strerror(errno));
            free(pathFile);
            exit(EXIT_FAILURE);
        }
        log_debug(logger, "Directorio del File <%s> creado", nombreFile);
    }

    if (!crearTag(pathFile, nombreTag)) {
        free(pathFile);
        return false;
    }

    free(pathFile);
    return true;
}

bool crearTag(char* pathFile, char* nombreTag){
    char *pathTag =  inicializarDirectorio(pathFile,nombreTag);
    char* pathLogicalBlocks = inicializarDirectorio(pathTag,"logical_blocks");

    struct stat st;
    if (stat(pathTag, &st) == 0) {
        log_debug(logger, "Error: File:Tag <%s:%s> ya existe (preexistente)", pathFile, nombreTag);
        free(pathFile);
        free(pathTag);
        return false;
    }

    crearMetaData(pathTag);

    free(pathTag);
    free(pathLogicalBlocks);
}

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

void inicializarConexiones() {
    listadoWorker = list_create();
    pthread_mutex_init(&mutexWorkers, NULL);
}

