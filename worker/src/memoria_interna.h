#ifndef MEMORIA_INTERNA_H
#define MEMORIA_INTERNA_H

#include "utils/config.h"
#include "globals.h"
#include "commons/bitarray.h"
#include "commons/string.h"
#include "conexiones.h"


// Variables globales
char* memoria = NULL;        // memoria principal
t_bitarray* bitmap = NULL;   // bitmap para páginas
int cant_frames = 0;
t_dictionary* tablasDePaginas = NULL; //la key es <FILE>:<TAG>

int cant_paginas;
void asignarCant_paginas(void);


//Mutex
pthread_mutex_t memoria_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t tabla_paginas_mutex = PTHREAD_MUTEX_INITIALIZER;

// FUNCIONES
void inicializarMemoriaInterna(void); // Hecho 
void inicializarDiccionarioDeTablas(void); // Hecho 
void eliminarMemoriaInterna(void); // Hecho 

void agreagarTablaPorFileTagADicionario(char* nombreFile, char* tag); // Hecho
TablaDePaginas* obtenerTablaPorFileYTag(const char* nombreFile, const char* tag); // Hecho

int obtenerMarcoLibre(void); // Hecho
void liberarMarco(int nro_marco); // Hecho
int obtenerNumeroDeMarco(const char* nombreFile, const char* tag, int numeroPagina); // Hecho
char* obtenerContenidoDelMarco(int nro_marco); // Hecho
 
char* traerMarcoDeStorage(char* nombreFile, char* tag, int numeroPagina); // ---Falta esperar storage---
void enviarMarcoAStorage(char* nombreFile, char* tag, int numeroPagina); // Hecho (a revisar)



// Lectura/Escritura desde la "Memoria Interna" 
char* leerContenidoDesdeOffset(   const char* nombreFile, const char* tag, int numeroMarco, int offset, int size); //Falta
void escribirContenidoDesdeOffset(const char* nombreFile, const char* tag, int numeroMarco, char* contenido, int offset, int size); //Falta

void* leerDesdeMemoriaPaginaCompleta(const char* nombreFile, const char* tag, int numeroMarco); //Falta
void escribirEnMemoriaPaginaCompleta(const char* nombreFile, const char* tag, int numeroMarco, char* contenidoPagina, int size); //Falta


// int obtenerNumeroPaginaDeFileTag(const char* nombreFile, const char* tag, int direccionBase); //---No se si es necesario---


// Algoritmos de reemplazo (devuelven la página reemplazada)
int ReemplazoLRU(); //Por Hacer         puede usar obtenerPaginaLibre(), reservarPagina(), liberarPagina(), agregarPaginaAProceso()
int ReemplazoCLOCKM(); //Por Hacer      puede usar obtenerPaginaLibre(), reservarPagina(), liberarPagina(), agregarPaginaAProceso()

#endif