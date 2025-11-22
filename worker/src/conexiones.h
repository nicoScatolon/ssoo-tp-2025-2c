#ifndef WORKER_CONEXIONES_H
#define WORKER_CONEXIONES_H

#include "utils/sockets.h"
#include "utils/logs.h"
#include "utils/paquete.h"
#include "globals.h"
#include "query_interpreter.h"


extern configWorker * configW;
void conexionConMaster(int workerId);
void conexionConMasterDesalojo(int workerId);
void esperarRespuesta();
void* escucharMaster();
int escucharStorage();
char* escucharStorageContenidoPagina();
void conexionConStorage(int workerId);
void* escucharDesalojo();


#endif