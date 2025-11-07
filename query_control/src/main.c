#include "main.h"

configQuery *configQ;
t_log* logger;
int main(int argc,char*argv[]){
    if (argc < 4) {
        printf("Uso: %s <config> <path> <prioridad>\n", argv[0]);
        return EXIT_FAILURE;
    }
    char* nombreConfiguracion = argv[1];
    char* path = argv[2];
    int prioridad = atoi(argv[3]);

    if (prioridad < 0) {
        printf("La prioridad debe ser un numero mayor o igual que 0\n");
        exit(EXIT_FAILURE);
    }
    configQ = malloc(sizeof(configQuery));
    iniciarConfiguracionQueryControl(nombreConfiguracion,configQ);
    logger = iniciar_logger("queryControl",configQ->logLevel);
    iniciarConexion(path,prioridad);
    signal(SIGINT, manejar_sigint);
    esperarRespuesta();
    //liberarQuery(logger,config);
}