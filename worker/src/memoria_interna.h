#ifndef MEMORIA_INTERNA_H
#define MEMORIA_INTERNA_H

#include "utils/config.h"
#include "globals.h"
#include <commons/bitarray.h>


// Variables globales
char* memoria = NULL;        // memoria principal
t_bitarray* bitmap = NULL;   // bitmap para páginas
int cant_frames = 0;
t_dictionary* tablasDePaginas = NULL; //la key es <FILE>:<TAG>

extern configWorker* configW;

//Mutex
pthread_mutex_t memoria_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t tabla_paginas_mutex = PTHREAD_MUTEX_INITIALIZER;

// FUNCIONES
void inicializarMemoriaInterna(void); // Hecho
void inicializarDiccionarioDeTablas(void); // Hecho
void eliminarMemoriaInterna(void); // Hecho

void agreagarTablaPorFileTagADicionario(char* nombreFile, char* tag); // Hecho
TablaDePaginas* obtenerTablaPorFileYTag(const char* nombreFile, const char* tag);

int obtenerMarcoLibre(void);
void liberarPagina(int nro_pagina);
void reservarPagina(int nro_pagina); 

int obtenerNumeroPaginaDeFileTag(const char* nombreFile, const char* tag, int direccionBase); //-------Por Hacer-------



// Lectura/Escritura desde la "Memoria Interna" 
char* leerContenidoDesdeOffset(const char* nombreFile, const char* tag, int numeroMarco, int size, int offset); 
void escribirContenidoDesdeOffset(const char* nombreFile, const char* tag, int numeroMarco, int offset, int size, char* contenido); 

void* leerDesdeMemoriaPaginaCompleta(const char* nombreFile, const char* tag, int numeroMarco); //Hecho
void escribirEnMemoriaPaginaCompleta(const char* nombreFile, const char* tag, int numeroMarco, char* contenidoPagina, int size);

int pedirMarco(const char* nombreFile, const char* tag, int numeroPagina);

// Algoritmos de reemplazo (devuelven la página reemplazada)
int ReemplazoLRU(); //Por Hacer         puede usar obtenerPaginaLibre(), reservarPagina(), liberarPagina(), agregarPaginaAProceso()
int ReemplazoCLOCKM(); //Por Hacer      puede usar obtenerPaginaLibre(), reservarPagina(), liberarPagina(), agregarPaginaAProceso()

#endif