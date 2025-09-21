#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H

#include "configuracion.h"
#include "utils/config.h"
#include "utils/globales.h"
#include <sys/stat.h>
#include <sys/types.h>
#include "globals.h"


typedef struct{
    char* hash;
    uint32_t numero;
    char* contenido;
}t_hash_block;

extern configStorage *configS;
extern configSuperBlock *configSB;

extern t_log* logger;

#include "estructuras.h"

void vaciarMemoria(void);
void liberarBloqueHash(t_hash_block * bloque);
bool existeHash(char * hash);
void escribirBloqueHash(t_hash_block *bloque);
void ocuparBloque(void);
void incrementarNumeroBloque(void);

#endif