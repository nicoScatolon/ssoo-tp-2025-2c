#include "inicializar.h"

//Inicializar archivo generico, no lo vamos a usar, pero sirven de referencias
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

void inicializarBitmap(char* path){
    inicializarArchivo(path,"bitmap",".bin","w+"); //provisorio
}

void inicializarBlocksHashIndex(char* path){
    inicializarArchivo(path,"blocks_hash_index",".config","w+"); //provisorio
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

void inicializarFileSystem(char* pathPhysicalBlocks) {
    if (configS->freshStart) {
        int cantBloques = configSB->FS_SIZE / configSB->BLOCK_SIZE;

        for (int i = 0; i < cantBloques; i++) {
            char nombreBloque[32];
            // Generar: block0000, block0001, ..., block9999
            snprintf(nombreBloque, sizeof(nombreBloque), "block%04d", i);

            inicializarArchivo(pathPhysicalBlocks, nombreBloque, "dat", "w+");
        }
    }
}

void inicializarMutex(void){
    pthread_mutex_init(&mutex_hash_block,NULL);
    pthread_mutex_init(&mutex_numero_bloque,NULL);
}

void inicializarEstructuras(void){
    char* path = configS->puntoMontaje;

    log_debug(logger,"Punto de montaje: %s",path);

    // Archivos
    //inicializarSuperblock(); //se debe hacer en las configs
    inicializarBitmap(path);
    inicializarBlocksHashIndex(path);

    // Directorios
    char* pathPhysicalBlocks = inicializarDirectorio(path, "physical_blocks"); //terminado
    char* pathFiles = inicializarDirectorio(path, "files"); //hacer

    inicializarFileSystem(pathPhysicalBlocks);

    log_debug(logger,"Estructuras inicializadas correctamente");

    free(path);
    free(pathPhysicalBlocks);
    free(pathFiles);
}
