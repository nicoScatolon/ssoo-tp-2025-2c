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

void iniciarPlanificacion(void) {
    pthread_t th;

    if (strcmp(configM->algoritmoPlanificacion, "FIFO") == 0) {
        pthread_create(&th, NULL, planificadorFIFO, NULL);
        pthread_detach(th);

    } else if (strcmp(configM->algoritmoPlanificacion, "PRIORIDADES") == 0) {
        pthread_create(&th, NULL, planificadorPrioridad, NULL);
        pthread_detach(th);

        // Aging (si corresponde)
        if (configM->tiempoAging > 0) {
            pthread_t th_aging;
            pthread_create(&th_aging, NULL, Aging, NULL);
            pthread_detach(th_aging);
        }

        pthread_t th_desalojo;
        pthread_create(&th_desalojo, NULL, hiloDesalojo, NULL);
        pthread_detach(th_desalojo);

    } else {
        log_warning(logger, "ALGORITMO_PLANIFICACION desconocido: %s. Uso FIFO.", configM->algoritmoPlanificacion);
        pthread_create(&th, NULL, planificadorFIFO, NULL);
        pthread_detach(th);
    }

    // Por si ya había READY/Workers antes de lanzar los hilos
    // NO CREO QUIZAS BORRO ESA LINEA
    despertar_planificador();
}