#ifndef UTILS_CLEANUP_H_
#define UTILS_CLEANUP_H_

#include "../commons/common.h"
#include "config.h"
#include "commons/log.h"
#include "commons/config.h"

void liberarQuery(t_log*logger, t_config* configQ);
void liberarMaster(t_log* logger, t_config* configM);
void liberarWorker(t_log*logger, t_config* configW);
void liberarStorage(t_log*logger, t_config* configS);

#endif