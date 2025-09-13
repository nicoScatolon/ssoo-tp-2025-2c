
#ifndef CONEXIONES_H
#define CONEXIONES_H
#include "globals.h"
#include "utils/sockets.h"
#include "utils/globales.h"
#include "planificadores.h"


void agregarQueryControl(char* path,int socketCliente, int prioridad);
void agregarWorker(int socketCliente,int idWorker);
void agregarQuery(char* path,int prioridad,int id);

uint32_t generarIdQueryControl();

void *escucharQueryControl(void* socketServidorVoid);
void *escucharWorker(void* socketServidorVoid);
void comprobacionModulo(modulo modulo_origen, modulo esperado, char *modulo, void*(*operacion)(void*), int socket_cliente);
void * operarQueryControl(void* socketClienteVoid);
void * operarWorker(void*socketClienteVoid);
void establecerConexiones();



void eliminarQueryControl(queryControl* queryC);
void eliminarWorker(worker* workerEliminar);
void eliminarQuery(query * queryEliminar);
#endif
