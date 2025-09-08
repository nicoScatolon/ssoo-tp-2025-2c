#ifndef WORKER_CONEXIONES_H
#define WORKER_CONEXIONES_H

#include "utils/sockets.h"
#include "utils/paquete.h"
#include "commons/log.h"

int conexionConMaster(configWorker* configW, t_log* logger);

#endif
