#ifndef GLOBALS_H
#define GLOBALS_H


#define MAX_PARAMETROS 4

#include "utils/config.h"
#include "utils/logs.h"
// #include <commons/collections/list.h>
// #include <pthread.h>


extern t_log* logger;

extern int socketMaster;
extern int socketStorage;


typedef enum {
    CREATE,
    TRUNCATE,
    WRITE,
    READ,
    TAG,
    COMMIT,
    FLUSH,
    DELETE,
    END,
    DESCONOCIDO
} tipo_instruccion_t;

typedef struct {
    tipo_instruccion_t tipo;
    char* parametro[MAX_PARAMETROS];
} instruccion_t;

typedef struct {
    char* path_query;
    int query_id;
    int pc;
    char** lineas_query;
    int total_lineas;
} contexto_query_t;

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




#endif