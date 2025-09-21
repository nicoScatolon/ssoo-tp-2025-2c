#ifndef MEMORIA_INTERNA_H
#define MEMORIA_INTERNA_H

#include "utils/config.h"
#include "globals.h"
#include <commons/bitarray.h>


// Variables globales
char* memoria = NULL;        // memoria principal
t_bitarray* bitmap = NULL;   // bitmap para páginas
int cant_paginas = 0;

extern configWorker* configW;

//Mutex
pthread_mutex_t memoria_mutex = PTHREAD_MUTEX_INITIALIZER;

// FUNCIONES
void inicializarMemoriaInterna(void); // Hecho
void eliminarMemoriaInterna(void); // Hecho

int obtenerPaginaLibre(void); // Hecho
void liberarPagina(int nro_pagina); // Hecho
void reservarPagina(int nro_pagina); // Hecho

// Lectura/Escritura desde la "Memoria Interna" (devuelven 0 ok, -1 error)
char* leerDesdeMemoriaByte(const char* file, const char* tag, int nro_pagina, int offset, void* buffer, size_t size);
char* escribirEnMemoriaByte(const char* file, const char* tag, int nro_pagina, int offset, const void* buffer, size_t size);

char* leerDesdeMemoriaPagina(const char* file, const char* tag, int nro_pagina, void* buffer, size_t size);
char* escribirEnMemoriaPagina(const char* file, const char* tag, int nro_pagina, const void* buffer, size_t size);

// Algoritmos de reemplazo (devuelven la página reemplazada)
int ReemplazoLRU();
int ReemplazoCLOCKM();

#endif