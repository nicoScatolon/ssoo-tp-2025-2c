#ifndef WORKER_CONEXIONES_H
#define WORKER_CONEXIONES_H

#include "utils/sockets.h"
#include "utils/logs.h"
#include "utils/paquete.h"
#include "globals.h"
#include "query_interpreter.h"


extern configWorker * configW;
void conexionConMaster(int idQuery);
void conexionConMasterDesalojo();
void esperarRespuesta();
void escucharMaster();
void escucharStorage();
char* escucharStorageContenidoPagina();
void conexionConStorage();
void* escucharDesalojo();


#endif