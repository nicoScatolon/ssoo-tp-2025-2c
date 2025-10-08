#ifndef MEMORIA_INTERNA_H
#define MEMORIA_INTERNA_H

#include "utils/config.h"
#include "globals.h"
#include <commons/bitarray.h>


// Variables globales
char* memoria = NULL;        // memoria principal
t_bitarray* bitmap = NULL;   // bitmap para páginas
int cant_frames = 0;
t_dictionary* tablasDePaginas = NULL; //la key es <FILE><TAG>

extern configWorker* configW;

//Mutex
pthread_mutex_t memoria_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t tabla_paginas_mutex = PTHREAD_MUTEX_INITIALIZER;

// FUNCIONES
void inicializarMemoriaInterna(void); // Hecho
void inicializarDiccionarioDeTablas(void); // Hecho
void eliminarMemoriaInterna(void); // Hecho

void agregarFiletagADiccionarioDeTablas(char* nombreFile, char* tag); // Hecho

int obtenerPaginaLibre(void); // Hecho
void liberarPagina(int nro_pagina); // Hecho
void reservarPagina(int nro_pagina); // Hecho
void agregarPaginaAProceso(const char* nombreFile, const char* tag, int nro_pagina); //-------Por Terminar-------

int obtenerPaginaDeFileTag(const char* nombreFile, const char* tag, int direccionBase); //-------Por Hacer-------
int pedirPagina(const char* nombreFile, const char* tag, int direccionBase); //-------Por Hacer-------



// Lectura/Escritura desde la "Memoria Interna" 
char leerDesdeMemoriaByte(const char* nombreFile, const char* tag, int nroPagina, int offset); //Hecho
void escribirEnMemoriaByte(const char* nombreFile, const char* tag, int nroPagina, int offset, char valor); //Hecho

void* leerDesdeMemoriaPaginaCompleta(const char* nombreFile, const char* tag, int nroPagina); //Hecho
void escribirEnMemoriaPaginaCompleta(const char* nombreFile, const char* tag, int nroPagina, void* contenidoPagina, size_t size); //Hecho

// Para el COMMIT
void* ObtenerTodasLasPaginasModificadas(); //-------Por Hacer-------

// Algoritmos de reemplazo (devuelven la página reemplazada)
int ReemplazoLRU(); //Por Hacer         puede usar obtenerPaginaLibre(), reservarPagina(), liberarPagina(), agregarPaginaAProceso()
int ReemplazoCLOCKM(); //Por Hacer      puede usar obtenerPaginaLibre(), reservarPagina(), liberarPagina(), agregarPaginaAProceso()

#endif