#ifndef MEMORIA_INTERNA_H
#define MEMORIA_INTERNA_H

#include "utils/config.h"
#include "commons/bitarray.h"
#include "commons/string.h"
#include "conexiones.h"
#include "algoritmosReemplazo.h"
#include "globals.h"

// Variables globales
extern char* memoria;
extern t_bitarray* bitmap;
extern int cant_marcos;
extern t_dictionary* tablasDePaginas;

// Mutex
extern pthread_mutex_t memoria_mutex;
extern pthread_mutex_t tabla_paginas_mutex;

// Inicialización y limpieza
void inicializarMemoriaInterna(void);
void inicializarDiccionarioDeTablas(void);
void eliminarMemoriaInterna(void);

// Gestión de tablas de páginas
TablaDePaginas* agreagarTablaPorFileTagADicionario(char* nombreFile, char* tag);
TablaDePaginas* obtenerTablaPorFileYTag(char* nombreFile, char* tag);
void actualizarMetadataTablaPagina(TablaDePaginas* tabla);

// Gestión de marcos
int obtenerMarcoLibre(void);
int obtenerMarcoReservado(char* keyAsignar, int numeroPagina);  // ← AGREGAR
int liberarMarcoVictima(char* nombreFileVictima, char* tagFileVictima, int pagina, int marco);
void asignarMarco(char* fileNameAsignar, char* tagFileAsignar, int pagina, int marco);
void asignarMarcoEntradaTabla(char* nombreFile, char* tag, int numeroPagina, int numeroMarco);
void liberarMarco(int nro_marco);
void ocuparMarco(int nro_marco);

// Acceso a memoria
int obtenerNumeroDeMarco(char* nombreFile, char* tag, int numeroPagina);
int obtenerMarcoDesdePagina(char* nombreFile, char* tag, int numeroPagina);
char* obtenerContenidoDelMarco(int nro_marco, int offset, int size);

// Comunicación con Storage
char* traerPaginaDeStorage(char* nombreFile, char* tag, int query_id, int numeroPagina);
int enviarPaginaAStorage(char* nombreFile, char* tag, int numeroPagina);

// Lectura/Escritura
char* leerContenidoDesdeOffset(char* nombreFile, char* tag, int numeroPagina, int numeroMarco, int offset, int size);
void escribirContenidoDesdeOffset(char* nombreFile, char* tag, int numeroPagina, int numeroMarco, char* contenido, int offset, int size);
void escribirEnMemoriaPaginaCompleta(char* nombreFile, char* tag, int numeroPagina, int marcoLibre, char* contenidoPagina, int size);
void escribirMarcoConOffset(int numeroMarco, char* contenido, int offset, int size);

// Algoritmos y utilidades
key_Reemplazo* ejecutarAlgoritmoReemplazo();
void aplicarRetardoMemoria(void);
int64_t obtener_tiempo_actual();
void inicializar_entrada(EntradaDeTabla* entrada, int numeroPagina);
void actualizar_acceso_pagina(EntradaDeTabla* entrada);
void modificar_pagina(EntradaDeTabla* entrada);

#endif