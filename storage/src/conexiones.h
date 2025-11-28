#ifndef CONEXIONES_H
#define CONEXIONES_H

#include "configuracion.h"
#include "utils/paquete.h"
#include "utils/config.h"
#include "utils/globales.h"
#include "estructuras.h"
#include "operaciones.h"

extern configStorage *configS;
extern configSuperBlock* configSB;
extern t_log* logger;

void establecerConexionesStorage(void);
void *escucharWorkers(void* socketServidorVoid);
void comprobacionModulo(modulo modulo_origen, modulo esperado, char *modulo, void*(*operacion)(void*), int socket_cliente);
void *operarWorkers(void*socketClienteVoid);
void incrementarWorkers(int workerId);
void decrementarWorkers(int workerId);
void registrarWorker(int socket, int workerId);
int obtenerYRemoverWorker(int socket);
#endif