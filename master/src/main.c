#include <utils/hello.h>

configMaster* configM;
t_log* logger;
int main(int argc, char* argv[]) {
    if (argc < 1)
    {
        printf("No se pasron los parametros necesarios, se necesita el nombre del archivo de configuracion\n");
        return EXIT_FAILURE;
    }
    char* nombreConfig = argv[1]; //no se si es el 0 o el 1

    iniciarConfiguracionMaster(nombreConfig, configM);
    *logger = iniciar_logger("master", configM->logLevel);
    establecerConexiones();
    //liberarMaster(logger, configM);
    return 0;
}
