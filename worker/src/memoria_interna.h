#ifndef MEMORIA_INTERNA_H
#define MEMORIA_INTERNA_H

#include "utils/config.h"
#include "globals.h"
#include <commons/bitarray.h>


// Variables globales
char* memoria = NULL;        // memoria principal
t_bitarray* bitmap = NULL;   // bitmap para páginas
int cant_paginas = 0;
t_dictionary* tablasDePaginas = NULL;

extern configWorker* configW;

//Mutex
pthread_mutex_t memoria_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t tabla_paginas_mutex = PTHREAD_MUTEX_INITIALIZER;

// FUNCIONES
void inicializarMemoriaInterna(void); // Hecho
void inicializarDiccionarioDeTablas(void); // Hecho
void eliminarMemoriaInterna(void); // Hecho

int obtenerPaginaLibre(void); // Hecho
void liberarPagina(int nro_pagina); // Hecho
void reservarPagina(int nro_pagina); // Hecho
void agregarPaginaAProceso(int pid, int nro_pagina); //Por Terminar

// Lectura/Escritura desde la "Memoria Interna" 
char leerDesdeMemoriaByte(const char* nombreFile, const char* tag, int nroPagina, int offset); //Hecho
void escribirEnMemoriaByte(const char* nombreFile, const char* tag, int nroPagina, int offset, char valor); //Hecho

void* leerDesdeMemoriaPaginaCompleta(const char* nombreFile, const char* tag, int nroPagina); //Hecho
void escribirEnMemoriaPaginaCompleta(const char* nombreFile, const char* tag, int nroPagina, void* contenidoPagina, size_t size); //Hecho

// Para el COMMIT
void* ObtenerTodasLasPaginasModificadas(); //Por Hacer

// Algoritmos de reemplazo (devuelven la página reemplazada)
int ReemplazoLRU(); //Por Hacer
int ReemplazoCLOCKM(); //Por Hacer

#endif