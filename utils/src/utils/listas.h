#ifndef LISTAS_H
#define LISTAS_H
#include <commons/collections/list.h>
#include <pthread.h>
typedef struct{
    t_list * lista;
    pthread_mutex_t mutex;
}t_list_mutex;

bool esListaVacia(t_list_mutex* lista);
void *listRemove(t_list_mutex* lista);
void listaAdd(void* elemento, t_list_mutex*lista);

#endif

