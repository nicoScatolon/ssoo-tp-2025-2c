
#ifndef CONEXIONES_H
#define CONEXIONES_H
#include "utils/sockets.h"
#include "utils/globales.h"
#include "utils/config.h"
#include "utils/logs.h"
#include "utils/paquete.h"
#include "globals.h"
#include "planificadores.h"
#include "qControl.h"
#include "workers.h"

uint32_t generarIdQueryControl();
void *escucharQueryControl(void* socketServidorVoid);
void *escucharWorker(void* socketServidorVoid);
void *escucharWorkerDesalojo(void* socketServidorVoid);
void *operarQueryControl(void* socketClienteVoid);
void *operarWorker(void*socketClienteVoid);
void establecerConexiones();
void comprobacionModulo(modulo modulo_origen, modulo esperado, char *modulo, void*(*operacion)(void*), int socket_cliente);
void eliminarQuery(query * queryEliminar);
void agregarQuery(char* path,int prioridad,int id);
void aumentarCantidadQueriesControl();
void aumentarGradoMultiprogramacion();
void disminuirCantidadQueriesControl();
void disminuirGradoMultiprogramacion();

#endif
