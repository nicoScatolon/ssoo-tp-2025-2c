#include "inicializar.h"
void inicializarListas() {
    
    pthread_mutex_init(&listaReady.mutex, NULL);
    listaReady.lista = list_create();

    pthread_mutex_init(&listaExecute.mutex, NULL);
    listaExecute.lista = list_create();

    pthread_mutex_init(&listaExit.mutex, NULL);
    listaExit.lista = list_create();

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

        // Aging (si corresponde)
        if (configM->tiempoAging > 0) {
            pthread_t hiloAging;
            pthread_create(&hiloAging, NULL, aging, NULL);
            pthread_detach(hiloAging);
        }


        pthread_create(&hiloDesalojo, NULL, evaluarDesalojo, NULL);
        pthread_detach(hiloDesalojo);

    } else {
        log_warning(logger, "ALGORITMO_PLANIFICACION desconocido: %s. Uso FIFO.", configM->algoritmoPlanificacion);
        exit (EXIT_FAILURE);
    }
}