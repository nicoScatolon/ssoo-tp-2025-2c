#ifndef UTILS_CLEANUP_H_
#define UTILS_CLEANUP_H_

#include "../commons/common.h"
#include "config.h"
#include "commons/log.h"
#include "commons/config.h"

void liberarQuery(t_log** logger, configQuery* configQ);
void liberarMaster(t_log** logger, configMaster* configM);
void liberarWorker(t_log** logger, configWorker* configW);
void liberarStorage(t_log** logger, configStorage* configS);

#endif