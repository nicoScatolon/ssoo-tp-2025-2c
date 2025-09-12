#include "main.h"

t_log* logger;
configStorage* configS;

int main(int argc, char*argv[]){
    if (argc < 2){
        printf("No se pasaron los parametros necesarios, se necesita el nombre del archivo de configuracion\n");
        return EXIT_FAILURE;
    }
    char* nombreConfig = argv[1]; 
    configS = malloc(sizeof(configStorage));
    iniciarConfiguracionStorage(nombreConfig, configS);
    logger = iniciar_logger("storage", configS->logLevel);
    //inicializarEstructuras();
    establecerConexionesStorage();
    pthread_exit(NULL);
}