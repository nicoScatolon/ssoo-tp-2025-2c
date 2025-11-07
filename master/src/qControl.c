#include "qControl.h"

int obtenerPosicionQCPorId(int idBuscado) {
    pthread_mutex_lock(&listaQueriesControl.mutex);
    for (int i = 0; i < list_size(listaQueriesControl.lista); i++) {
        queryControl* elem = (queryControl*) list_get(listaQueriesControl.lista, i);
        if (elem->queryControlID == idBuscado) {
            pthread_mutex_unlock(&listaQueriesControl.mutex);
            return i;
        }
    }
    pthread_mutex_unlock(&listaQueriesControl.mutex);
    return -1; 
}

queryControl* buscarQueryControlPorId(int idQuery){
    pthread_mutex_lock(&listaQueriesControl.mutex);
    for (int i = 0; i < list_size(listaQueriesControl.lista); i++)
    {
        queryControl * queryC = list_get(listaQueriesControl.lista,i);
        if (queryC->queryControlID == idQuery)
        {
            pthread_mutex_unlock(&listaQueriesControl.mutex);
            return queryC;
        }
        
    }
    pthread_mutex_unlock(&listaQueriesControl.mutex);
    return NULL;
}
queryControl* buscarQueryControlPorSocket(int socket){
    pthread_mutex_lock(&listaQueriesControl.mutex);
    for (int i = 0; i < list_size(listaQueriesControl.lista); i++) {
        queryControl* qc = list_get(listaQueriesControl.lista, i);
        if (qc->socket == socket) {
            pthread_mutex_unlock(&listaQueriesControl.mutex);
            return qc;
        }
    }
    pthread_mutex_unlock(&listaQueriesControl.mutex);
    return NULL;
}
void eliminarQueryControl(queryControl* queryC){

    int posicion = obtenerPosicionQCPorId(queryC->queryControlID);
    pthread_mutex_lock(&listaQueriesControl.mutex);
    list_remove(listaQueriesControl.lista,posicion);
    pthread_mutex_unlock(&listaQueriesControl.mutex);


    pthread_mutex_destroy(&queryC->mutex);
    free(queryC->path);
    free(queryC);

}