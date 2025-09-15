#include <main.h>
configMaster* configM;
t_log* logger;
int main(int argc, char* argv[]) {
    if (argc < 2)
    {
        printf("No se pasaron los parametros necesarios, se necesita el nombre del archivo de configuracion\n");
        return EXIT_FAILURE;
    }
    char* nombreConfig = argv[1]; 
    configM = malloc(sizeof(configMaster));
    iniciarConfiguracionMaster(nombreConfig, configM);
    inicializarListas();
    logger = iniciar_logger("master", configM->logLevel);
    establecerConexiones();
    iniciarPlanificacion();
    pthread_exit(NULL);
    //liberarMaster(logger, configM);
}
