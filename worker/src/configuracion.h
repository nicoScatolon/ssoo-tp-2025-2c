#ifndef WORKER_CONEXIONES_H
#define WORKER_CONEXIONES_H

#include "utils/sockets.h"
#include "utils/logs.h"
#include "utils/paquete.h"
#include "globals.h"

int conexionConMaster(void);

void escucharMaster(int socketMaster);

#endif