#ifndef INICIALIZAR_H
#define INICIALIZAR_H
#include "globals.h"
#include "utils/globales.h"
#include "estructuras.h"

void inicializarEstructuras(void);
void inicializarMutex(void);
int inicializarBitmap(const char* bitmap_path);

#endif