#include "inicializar.h"
void inicializarListas() {
    
    pthread_mutex_init(&listaReady.mutex, NULL);
    listaReady.lista = list_create();

    pthread_mutex_init(&listaExecute.mutex, NULL);
    listaExecute.lista = list_create();

    pthread_mutex_init(&listaWorkers.mutex, NULL);
    listaWorkers.lista = list_create();

    pthread_mutex_init(&listaQueriesControl.mutex, NULL);
    listaQueriesControl.lista = list_create();
}

void iniciarPlanificacion() {
    if (strcmp(configM->algoritmoPlanificacion, "FIFO") == 0) {
        pthread_t hiloPlanificadorFIFO;
        pthread_create(&hiloPlanificadorFIFO, NULL, planificadorFIFO, NULL);
        pthread_detach(hiloPlanificadorFIFO);

    } else if (strcmp(configM->algoritmoPlanificacion, "PRIORIDADES") == 0) {
        pthread_t hiloPlanificadorPrioridares,hiloDesalojo;
        pthread_create(&hiloPlanificadorPrioridares, NULL, planificadorPrioridad, NULL);
        pthread_detach(hiloPlanificadorPrioridares);
      
        pthread_create(&hiloDesalojo, NULL, evaluarDesalojo, NULL);
        pthread_detach(hiloDesalojo);
    } else {
        log_warning(logger, "ALGORITMO_PLANIFICACION desconocido: %s.", configM->algoritmoPlanificacion);
        exit (EXIT_FAILURE);
    }
}
void inicializarSemaforos(){
    sem_init(&sem_ready,0,0);
    sem_init(&sem_execute,0,0);
    sem_init(&sem_workers_libres,0,0);
    sem_init(&sem_desalojo,0,0);
}