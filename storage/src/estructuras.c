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

//Se carga la configuracion del superblock, si no existe lo crea con valores por defecto
void inicializarSuperblock() {
    char* path = configS->puntoMontaje;

    char superblockPath[512];
    snprintf(superblockPath, sizeof(superblockPath), "%s/superblock.config", path);

    t_config* superblockConfig = config_create(superblockPath);
    if (!superblockConfig) {
        // Archivo no existe o error al abrirlo, lo creamos con valores por defecto
        FILE* archivo = fopen(superblockPath, "w");
        if (!archivo) {
            log_error(logger, "No se pudo crear superblock.config: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        fclose(archivo);

        superblockConfig = config_create(superblockPath);
        if (!superblockConfig) {
            log_error(logger, "No se pudo inicializar superblock.config");
            exit(EXIT_FAILURE);
        }

        // Valores por defecto
        config_set_value(superblockConfig, "FS_SIZE", "4096");
        config_set_value(superblockConfig, "BLOCK_SIZE", "128");
        config_save(superblockConfig);
    }

    log_info(logger, "Superblock cargado correctamente");
    config_destroy(superblockConfig);
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
    inicializarSuperblock();
    inicializarBitmap();
    inicializarBlocksHashIndex();

    // Directorios
    inicializarDirectorio("physical_blocks");
    inicializarDirectorio("files");

    log_debug(logger,"Estructuras inicializadas correctamente");
}



void vaciarMemoria(){

}


