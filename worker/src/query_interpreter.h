#ifndef QUERY_INTERPRETER_H
#define QUERY_INTERPRETER_H

#include "globals.h"
#include "utils/config.h"
#include "utils/logs.h"
#include "utils/paquete.h"
#include <string.h>
#include <stdlib.h>

// Tipos de instrucciones
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
    char** parametros;
} instruccion_t;

typedef struct {
    char* path_query;
    int query_id;
    int pc;
    char** lineas_query;
    int total_lineas;
} contexto_query_t;

contexto_query_t* cargarQuery(char* path, int query_id, int pc_inicial);
instruccion_t* parsearInstruccion(char* linea);
void ejecutarInstruccion(instruccion_t* instruccion, contexto_query_t* contexto);
void ejecutarQuery(contexto_query_t* contexto);
void liberarInstruccion(instruccion_t* instruccion);
void liberarContextoQuery(contexto_query_t* contexto);

tipo_instruccion_t obtenerTipoInstruccion(char* nombre);
void aplicarRetardoMemoria();
void dividirFileYTag(char* file_tag, char** nombre_file, char** tag);

#endif