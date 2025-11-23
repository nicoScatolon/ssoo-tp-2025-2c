#include "globals.h"

// Declare or extern configW if it's defined elsewhere
extern configWorker* configW;

int calcularPaginaDesdeDireccionBase(int direccionBase){
    int tamBloque = configW->BLOCK_SIZE;
    return direccionBase / tamBloque;
}   

int calcularOffsetDesdeDireccionBase(int direccionBase){
    int tamBloque = configW->BLOCK_SIZE;
    return direccionBase % tamBloque;
}


// libreria interna manejar vectorpagina
VectorPaginaXMarco* vector_pxm_create(int capacidad){
    VectorPaginaXMarco* vector = malloc(sizeof(VectorPaginaXMarco));
    vector->elementos = calloc(capacidad, sizeof(PaginaXMarco));
    vector->capacidad = capacidad;
    vector->cantidad = 0;
    return vector;
}

void vector_pxm_destroy(VectorPaginaXMarco* vector){
    for (int i = 0; i < vector->capacidad; i++) {
        free(vector->elementos[i].key);
    }

    free(vector->elementos);
    free(vector);
}

PaginaXMarco* vector_pxm_get(VectorPaginaXMarco* vector, int nroMarco){
    if (!vector || nroMarco < 0 || nroMarco >= vector->capacidad) {
        return NULL; 
    }
    return &vector->elementos[nroMarco];
}


bool vector_pxm_addIndex(VectorPaginaXMarco* vector, int numeroPagina, char* key, int index){
    if (!vector || index < 0 || index >= vector->capacidad)
        return false;       
    
    PaginaXMarco elemento = paginaXMarco_create(numeroPagina, key);
    
    vector->elementos[index] = elemento;

    if (index >= vector->cantidad)
        vector->cantidad = index + 1;

    return true;
}


//crea una entrada y con el contenido de numero de pagina y la key
PaginaXMarco paginaXMarco_create(int nroPagina, char* key){
    PaginaXMarco pxm;
    pxm.nroPagina = nroPagina;
    pxm.key = strdup(key);
    return pxm;
}