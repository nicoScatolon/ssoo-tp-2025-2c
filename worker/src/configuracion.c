#include "configuracion.h"


void iniciarConfiguracionWorker(char*nombreConfig ,configWorker* configWorker){
    t_config* config = iniciarConfig("worker",nombreConfig);
    *configW = agregarConfiguracionWorker(config);
}

sem_t sem_hayInterrupcion;

// Forward declaration of asignarCant_paginas

void inicializarCosas(){
    sem_init(&sem_hayInterrupcion, 1, 1);
    asignarCant_paginas();
}
