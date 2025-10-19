#include "configuracion.h"

void iniciarConfiguracionWorker(char*nombreConfig ,configWorker* configWorker){
    t_config* config = iniciarConfig("worker",nombreConfig);
    *configW = agregarConfiguracionWorker(config);
}

sem_t sem_hayInterrupcion;



void inicializarCosas(){
    pthread_t hilo_desalojo;
    pthread_create(&hilo_desalojo,NULL,escucharDesalojo,NULL);
    pthread_detach(hilo_desalojo);
    sem_init(&sem_hayInterrupcion, 0, 0); 
    asignarCant_paginas();
}

