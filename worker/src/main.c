#include "main.h"

configWorker* configW;
t_log* logger;



int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("No se pasaron los parametros necesarios, se necesita el nombre del archivo de configuracion\n");
        return EXIT_FAILURE;
    }
    // else if (argv[2] >= 0){ //convertir el argv[2] de string a int
    //     printf("No se indico ID de worker correcto, debe ser mayor o igual que 0. ");
    //     return EXIT_FAILURE;
    // }

    char* archivoConfig = argv[1];
    int workerId = atoi(argv[2]);

    configW = malloc(sizeof(configWorker));
    iniciarConfiguracionWorker(archivoConfig,configW);
    char* nombreLog = string_from_format("worker_%d", workerId);
    logger = iniciar_logger(nombreLog, configW->logLevel);

    conexionConMaster(workerId);
    conexionConStorage(workerId);
    inicializarEstructuras();
    
    conexionConMasterDesalojo(workerId);
    
    
    escucharMaster(); 
    

    return 0;
}
