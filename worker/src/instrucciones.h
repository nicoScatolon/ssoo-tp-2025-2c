#ifndef INSTRUCCIONES_H
#define INSTRUCCIONES_H

#include "../../utils/commons/common.h"
#include "query_interpreter.h"
#include "utils/globales.h"
#include "conexiones.h"
#include "globals.h"
#include "utils/paquete.h"
#include "utils/logs.h"
#include "utils/sockets.h"
#include <stdlib.h>
#include <string.h> 



void ejecutar_create(char* nombreFile, char* tagFile);  
void ejecutar_truncate(char* nombreFile, char* tagFile, int size);
void ejecutar_write(char* nombreFile, char* tagFile, int direccionBase, char* contenido);
void ejecutar_read(char* nombreFile, char* tagFile, int direccionBase, int size, contexto_query_t* contexto);
void ejecutar_tag(char* nombreFileOrigen, char* tagOrigen, char* nombreFileDestino, char* tagDestino);
void ejecutar_commit(char* nombreFile, char* tagFile);
void ejecutar_flush(void);
void ejecutar_delete(char* nombreFile, char* tagFile);
void ejecutar_end(contexto_query_t* contexto);    


#endif