#include <main.h>



int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("No se pasaron los parametros necesarios, se necesita el nombre del archivo de configuracion\n");
        return EXIT_FAILURE;
    }
    else if (argc[2] >= 0){
        printf("No se indico ID de worker correcto, debe ser mayor o igual que 0. ");
        return EXIT_FAILURE;
    }

    char* archivoConfig = argv[1];
    char* workerID = argv[2];

    configW = malloc(sizeof(config));
    iniciarConfiguracionWorker(archivoConfig,configW);

    logger = iniciar_logger("worker", configW->logLevel);
    
    log_info(logger, "## Iniciando Worker...");

    int socketMaster = conexionConMaster();

    escucharMaster(socketMaster);

    log_info(logger, "## Finalizando Worker.");
    return EXIT_SUCCESS;

    return 0;
}
