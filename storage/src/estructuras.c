#include "estructuras.h"

//-------- INICIALIZACION DE ESTRUCTURAS DE ALMACENAMIENTO --------
//inicializar arhivos genericos, sirve de guia pero no lo usamos
void inicializarArchivo(char* nombreArchivo){
    char* path = configS->puntoMontaje;

    char archivoPath[512];
    snprintf(archivoPath, sizeof(archivoPath), "%s/%s", path, nombreArchivo);

    // Abrir/crear archivos
    FILE* archivo = fopen(archivoPath, "a+");
    if (archivo == NULL) {
        log_error(logger, "No se pudo crear/abrir %s: %s", nombreArchivo, strerror(errno));
        exit(EXIT_FAILURE);
    }
    fclose(archivo);
}


void inicializarBitmap(){
    //Hacer
}

void inicializarBlocksHashIndex(){
    //Hacer
}


void inicializarDirectorio(char* nombreDirectorio) {
    char* path = configS->puntoMontaje;

    char dirPath[512];
    snprintf(dirPath, sizeof(dirPath), "%s/%s", path, nombreDirectorio);

    // mkdir devuelve 0 si creó el directorio, -1 si hubo error
    if (mkdir(dirPath, 0777) == -1) {
        if (errno != EEXIST) { // si el directorio ya existe, no es error
            log_error(logger, "No se pudo crear el directorio %s: %s", nombreDirectorio, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}

void inicializarEstructuras(){
    char* path = configS->puntoMontaje;

    log_debug(logger,"Punto de montaje: %s",path);

    // Archivos
    //inicializarSuperblock(); //se debe hacer en las configs
    inicializarBitmap();
    inicializarBlocksHashIndex();

    // Directorios
    inicializarDirectorio("physical_blocks");
    inicializarDirectorio("files");

    log_debug(logger,"Estructuras inicializadas correctamente");
}


void vaciarMemoria(){

}


