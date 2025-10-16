#include "configuracion.h"

void iniciarConfiguracionWorker(char*nombreConfig ,configWorker* configWorker){
    t_config* config = iniciarConfig("worker",nombreConfig);
    *configW = agregarConfiguracionWorker(config);
}

sem_t sem_hayInterrupcion;

// void inicializarCosas(){
//     sem_init(&sem_hayInterrupcion, 1, 1);
// }

void inicializarCosas(){
    sem_init(&sem_hayInterrupcion, 0, 0); // Bloqueado inicialmente

}

// void inicializarHilos(int ID){
//     pthread_t hilo_master, hilo_storage;
    
//     if (pthread_create(&hilo_master, NULL, escucharMaster, NULL) != 0) {
//         log_error(logger, "Error al crear hilo de escucha del Master");
//         return EXIT_FAILURE;
//     }
    
//     if (pthread_create(&hilo_storage, NULL, escucharStorage, NULL) != 0) {
//         log_error(logger, "Error al crear hilo de escucha del Storage");
//         return EXIT_FAILURE;
//     }

//     log_info(logger, "Worker %d iniciado correctamente", ID);

//     // Esperar a los hilos
//     pthread_detach(hilo_master);
//     pthread_detach(hilo_storage);
// }