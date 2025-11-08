#ifndef WORKERS_H
#define WORKERS_H

#include "globals.h"
#include "conexiones.h"

void enviarQueryAWorker(worker* workerElegido,char* path,int PC,int queryID);
void liberarWorker(worker* w);
void eliminarWorker(worker* workerEliminar);
worker*obtenerWorkerLibre();
worker*buscarWorkerPorQueryId(int idQuery);
worker*buscarWorkerPorSocket(int socket);
worker* buscarWorkerPorId(int idBuscado);
int obtenerPosicionWPorId(int idBuscado);
bool hayWorkerLibre();
#endif

