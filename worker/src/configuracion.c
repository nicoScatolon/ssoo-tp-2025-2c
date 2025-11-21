#include "configuracion.h"

void iniciarConfiguracionWorker(char*nombreConfig ,configWorker* configWorker){
    t_config* config = iniciarConfig("worker",nombreConfig);
    *configW = agregarConfiguracionWorker(config);
}

sem_t sem_hayInterrupcion;



void inicializarEstructuras(){
    sem_init(&sem_hayInterrupcion, 0, 0); 
    // asignarCant_paginas();
    inicializarMemoriaInterna();
    inicializarDiccionarioDeTablas();
}

