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


void inicializarBlocksHashIndex(char* path) {
    char archivoPath[512];
    snprintf(archivoPath, sizeof(archivoPath), "%s/blocks_hash_index.config", path);

    // Si ya existe, no hacer nada
    if (access(archivoPath, F_OK) == 0) {
        log_debug(logger, "Archivo blocks_hash_index.config ya existe, no se sobrescribe");
        return;
    }

    // Crear archivo vacío
    inicializarArchivo(path, "blocks_hash_index", "config", "w");
    log_debug(logger, "Archivo blocks_hash_index.config inicializado correctamente");
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


void levantarFileSystem(){
    if (configS->freshStart){
        pathBloquesFisicos = inicializarDirectorio(configS->puntoMontaje,"physical_blocks");
        
        inicializarBitmap(configS->puntoMontaje);
        inicializarBlocksHashIndex(configS->puntoMontaje);
        inicializarBloquesFisicos(pathBloquesFisicos);
        
        
        pathFiles = inicializarDirectorio(configS->puntoMontaje, "files");
        crearFile("initial_file","BASE");

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
    agregarBloque(pathTag,0);
    cambiarEstadoMetaData(pathTag);
    
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
    fprintf(archivo, "BLOCKS=%s\n", bloquesIniciales);
    fprintf(archivo, "ESTADO=%s\n", estadoInicial);

    fclose(archivo);
    free(pathMetadata);
}
void cambiarEstadoMetaData(char* pathTag) {
    char* pathMetadata = string_from_format("%s/metadata.config", pathTag);

    t_config* config = config_create(pathMetadata);
    if (config == NULL) {
        fprintf(stderr, "Error: no se pudo abrir %s\n", pathMetadata);
        free(pathMetadata);
        return;
    }
    config_set_value(config, "ESTADO", "COMMITED");
    config_save(config);
    config_destroy(config);
    free(pathMetadata);
}
void agregarBloque(char* pathTag, int nuevoBloque) {
    char* pathMetadata = string_from_format("%s/metadata.config", pathTag);
    t_config* config = config_create(pathMetadata);

    char** bloquesActuales = config_get_array_value(config, "BLOQUES");

    char* nuevoValor = string_new();
    string_append(&nuevoValor, "[");

    // concatenar bloques existentes
    if (bloquesActuales != NULL) {
        for (int i = 0; bloquesActuales[i] != NULL; i++) {
            string_append(&nuevoValor, bloquesActuales[i]);
            string_append(&nuevoValor, ",");
        }
    }

    char* numStr = string_from_format("%d", nuevoBloque);
    string_append(&nuevoValor, numStr);
    string_append(&nuevoValor, "]");

    config_set_value(config, "BLOQUES", nuevoValor);
    config_save(config);

    free(numStr);
    free(nuevoValor);
    free(pathMetadata);
    config_destroy(config);

}