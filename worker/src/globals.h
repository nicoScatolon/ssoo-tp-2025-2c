#ifndef GLOBALS_H
#define GLOBALS_H


#define MAX_PARAMETROS 4

#include "utils/config.h"
#include "utils/logs.h"
#include "configuracion.h"
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

extern contexto_query_t* contexto;

typedef struct {
    int         numeroFrame;
    int         numeroPagina;
    t_temporal  ultimoAcceso;
    bool        bitModificado;
    bool        bitUso;
    bool        bitPresencia;
} EntradaDeTabla;

typedef struct {
    EntradaDeTabla  *entradas;                   // array indexado por número de página virtual (PV)
    int                 capacidadEntradas;      // cuantos slots están reservados (p. ej. 16, 32)
    int                 cantidadEntradasUsadas; // opcional (para métricas)
    int                 paginasPresentes;       // cantidad de entradas con bitPresencia == true
    bool                hayPaginasModificadas;  // true si existe al menos una página con bitModificado==true
    char               *keyProceso;           // strdup("file:tag") — útil para logs
} TablaDePaginas;


int calcularPaginaDesdeDireccionBase(int direccionBase);
int calcularOffsetDesdeDireccionBase(int direccionBase);




#endif
