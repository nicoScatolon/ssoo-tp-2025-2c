#ifndef INSTRUCCIONES_H
#define INSTRUCCIONES_H

#include "../../utils/commons/common.h"
#include "memoria_interna.h"
#include "query_interpreter.h"
#include "utils/globales.h"
// #include "conexiones.h"
#include "globals.h"
#include "utils/paquete.h"

#include "utils/paquete.h"
#include "utils/logs.h"
#include "utils/sockets.h"
#include <stdlib.h>
#include <string.h> 



bool ejecutar_create(char* nombreFile, char* tagFile, int query_id);  
bool ejecutar_truncate(char* nombreFile, char* tagFile, int size, int query_id);
bool ejecutar_write(char* nombreFile, char* tagFile, int direccionBase, char* contenido, contexto_query_t* contexto);
bool ejecutar_read(char* nombreFile, char* tagFile, int direccionBase, int size, contexto_query_t* contexto);
bool ejecutar_tag(char* nombreFileOrigen, char* tagOrigen, char* nombreFileDestino, char* tagDestino, int query_id);
bool ejecutar_commit(char* nombreFile, char* tagFile, int query_id);
bool ejecutar_flush(char* fileName, char* tagFile, int query_id);
bool ejecutar_delete(char* nombreFile, char* tagFile, int query_id);
bool ejecutar_end(contexto_query_t* contexto);

void notificarMasterError(char* mensajeError);
#endif