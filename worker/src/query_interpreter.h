#ifndef QUERY_INTERPRETER_H
#define QUERY_INTERPRETER_H




#include "instrucciones.h"

#include "globals.h"
#include "configuracion.h"
// #include "conexiones.h"
#include "utils/config.h"
#include "utils/logs.h"
#include "utils/paquete.h"
#include <string.h>
#include <stdlib.h>

// Tipos de instrucciones




contexto_query_t* cargarQuery(char* path, int query_id, int pc_inicial);
instruccion_t* parsearInstruccion(char* linea);
void ejecutarInstruccion(instruccion_t* instruccion, contexto_query_t* contexto);
void ejecutarQuery(contexto_query_t* contexto);
void liberarInstruccion(instruccion_t* instruccion);
void liberarContextoQuery(contexto_query_t* contexto);

char* ObtenerNombreFileYTag(const char* fileTagText, char** fileOut, char** tagOut);


tipo_instruccion_t obtenerTipoInstruccion(char* nombre);
void aplicarRetardoMemoria();

void desalojarQuery(int idQuery, opcode motivo);




#endif