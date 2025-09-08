#include "planificadores.h"


// void planificarConFIFO(){
//     // Primero deberiamos tener en cuenta de que haya al menos un query_Control conectado, 
//     // Osea cuando se conecta una queryControl deberiamos tener pthread_cond_unlock
//     //Luego deberiamos ver si hay un worker disponible
//     // En caso afirmativo mandamos a ejecutar esa query al primer worker que encontremos
//     // En caso negativo la query deberia esperar en la lista
// }

worker * workerLibre(){
    pthread_mutex_lock(&mutex_lista_workers);
    for (int i = 0; i <list_size(workers); i++)
    {
        worker * workerLibre = list_get(workers,i);
        pthread_mutex_lock(&workerLibre->mutex);
        if(!workerLibre->ocupado){
            workerLibre->ocupado = true;
            pthread_mutex_unlock(&workerLibre->mutex);
            pthread_mutex_unlock(&mutex_lista_workers);
            return workerLibre;
        }
        pthread_mutex_unlock(&workerLibre->mutex);
    }
    pthread_mutex_unlock(&mutex_lista_workers);
    return NULL;
}

