#include "listas.h"

bool esListaVacia(t_list_mutex* l) {
    bool vacia;
    pthread_mutex_lock(&l->mutex);
    vacia = list_is_empty(l->lista);
    pthread_mutex_unlock(&l->mutex);
    return vacia;
}
void listaAdd(void* elemento, t_list_mutex* l) {
    pthread_mutex_lock(&l->mutex);
    list_add(l->lista, elemento);
    pthread_mutex_unlock(&l->mutex);
}

void listRemoveElement(t_list_mutex* l, void* elemento) {
    pthread_mutex_lock(&l->mutex);
    list_remove_element(l->lista, elemento);
    pthread_mutex_unlock(&l->mutex);
}

void * listRemove(t_list_mutex* l){
    pthread_mutex_lock(&l->mutex);
    void *elemento = NULL;
    if (!list_is_empty(l->lista)){
        elemento = list_remove(l->lista,0);
    }
    pthread_mutex_unlock(&l->mutex);
    return elemento;
}

