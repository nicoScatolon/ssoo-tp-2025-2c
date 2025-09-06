#include "main.h"

configQuery *configQ;
t_log* logger;
int main(int argc,char*argv[]){
    if (argc < 4)
    {
        printf("No se pasaron los parametros necesarios\n");
        return EXIT_FAILURE;
    } else if (argv[2] < 0)
    {
        printf("La prioridad debe ser un numero mayor a 0\n");
        return EXIT_FAILURE;
    }
    
    char* nombreConfiguracion = argv[1];
    char* path = argv[2];
    int  prioridad = atoi(argv[3]);
    configQ=malloc(sizeof(configQuery));
    iniciarConfiguracionQueryControl(nombreConfiguracion,configQ);
    logger = iniciar_logger("queryControl",configQ->logLevel);
    iniciarConexion(path,prioridad);
    //esperarRespuesta();
    //liberarQuery(logger,config);
}