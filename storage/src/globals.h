#ifndef GLOBALS_H
#define GLOBALS_H
#include "main.h"
#include "inicializar.h"
extern t_log* logger;
extern configStorage* configS;
extern configSuperBlock* configSB;
int numeroBloque;
pthread_mutex_t mutex_hash_block;
pthread_mutex_t mutex_numero_bloque;
#endif