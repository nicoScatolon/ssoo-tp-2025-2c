#ifndef CONEXIONES_H
#define CONEXIONES_H
#include "globals.h"
#include "utils/sockets.h"
#include "utils/globales.h"


void agregarQueryControl(char* path,int socketCliente, int prioridad);
void agregarWorker(int socketCliente);
void agregarQuery(char* path,int prioridad,int id);
uint32_t generarIdQueryControl();
uint32_t generarIdWorker();
uint32_t generarIdQuery();
void *escucharQueryControl(void* socketServidorVoid);
void comprobacionModulo(modulo modulo_origen, modulo esperado, char *modulo, void*(*operacion)(void*), int socket_cliente);
void * operarQueryControl(void* socketClienteVoid);

void establecerConexiones();
#endif
