
#ifndef CONEXIONES_H
#define CONEXIONES_H
#include "globals.h"
#include "utils/sockets.h"
#include "utils/globales.h"
#include "planificadores.h"
#include "utils/config.h"
#include "utils/logs.h"
#include "utils/paquete.h"
#include "globals.h"

uint32_t generarIdQueryControl();

queryControl* buscarQueryControlPorSocket(int socket);
worker* buscarWorkerPorSocket(int socket);
query* sacarDeExecutePorId(int queryID);

void *escucharQueryControl(void* socketServidorVoid);
void *escucharWorker(void* socketServidorVoid);
void * operarQueryControl(void* socketClienteVoid);
void * operarWorker(void*socketClienteVoid);

void establecerConexiones();
void comprobacionModulo(modulo modulo_origen, modulo esperado, char *modulo, void*(*operacion)(void*), int socket_cliente);

void eliminarQueryControl(queryControl* queryC);
void eliminarWorker(worker* workerEliminar);
void eliminarQuery(query * queryEliminar);

void agregarQueryControl(char* path,int socketCliente, int prioridad);
void agregarWorker(int socketCliente,int idWorker);
void agregarQuery(char* path,int prioridad,int id);

#endif
