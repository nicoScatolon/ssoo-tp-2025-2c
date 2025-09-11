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