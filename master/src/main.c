#include <main.h>
configMaster* configM;
t_log* logger;
int main(int argc, char* argv[]) {
    if (argc < 2)
    {
        printf("No se pasaron los parametros necesarios, se necesita el nombre del archivo de configuracion\n");
        return EXIT_FAILURE;
    }
    char* nombreConfig = argv[1]; //no se si es el 0 o el 1
    configM = malloc(sizeof(configMaster));
    iniciarConfiguracionMaster(nombreConfig, configM);
    logger = iniciar_logger("master", configM->logLevel);
    establecerConexiones();
    pthread_exit(NULL);
    //liberarMaster(logger, configM);
}
