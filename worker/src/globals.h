#ifndef GLOBALS_H
#define GLOBALS_H

#include "utils/config.h"
#include "utils/logs.h"
// #include <commons/collections/list.h>
// #include <pthread.h>


extern t_log* logger;


typedef struct {
    uint32_t numeroPagina;
    t_temporal ultimoAcceso;
    bool bitModificado;
    bool bitUso;
    bool bitPresencia;
} EntradaTablaPagina;

typedef struct {
    EntradaTablaPagina* entradas; 
    // uint32_t numPaginas;
} TablaPagina;


t_dictionary* tablasDePaginas = NULL;

#endif