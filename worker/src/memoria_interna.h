#ifndef MEMORIA_INTERNA_H
#define MEMORIA_INTERNA_H

#include "utils/config.h"
#include "commons/bitarray.h"
#include "commons/string.h"
#include "conexiones.h"
#include "algoritmosReemplazo.h"
#include "globals.h"



// Variables globales
extern char* memoria ;        // memoria principal
extern t_bitarray* bitmap ;   // bitmap para páginas
extern int cant_marcos;
extern t_dictionary* tablasDePaginas; //la key es <FILE>:<TAG>

extern int cant_paginas;

// extern void asignarCant_paginas(void);


void aplicarRetardoMemoria(void);

//Mutex
extern pthread_mutex_t memoria_mutex;
extern pthread_mutex_t tabla_paginas_mutex;

// FUNCIONES
void inicializarMemoriaInterna(void); // Hecho 
void inicializarDiccionarioDeTablas(void); // Hecho 
void eliminarMemoriaInterna(void); // Hecho 

TablaDePaginas* agreagarTablaPorFileTagADicionario(char* nombreFile, char* tag); // Hecho
TablaDePaginas* obtenerTablaPorFileYTag(char* nombreFile, char* tag); // Hecho
void actualizarMetadataTablaPagina(TablaDePaginas* tabla);

void escribirMarcoConOffset(int numeroMarco, char* contenido, int offset, int size);

int obtenerMarcoLibre(void); // Hecho
int liberarMarcoVictimaYAsignar(key_Reemplazo* keyVictima, char* keyAsignar, int numeroPagina);

void liberarMarco(int nro_marco); // Hecho
void ocuparMarco(int nro_marco);
int obtenerMarcoDesdePagina(char* nombreFile, char* tag, int numeroPagina); // Hecho
char* obtenerContenidoDelMarco(int nro_marco, int offset, int size);

int obtenerMarcoLibreYReservado(char* keyAsignar, int numeroPagina);

int obtenerNumeroDeMarco(char* nombreFile, char* tag, int numeroPagina); // Hecho
 
char* traerPaginaDeStorage(char* nombreFile, char* tag, int query_id, int numeroPagina);

int enviarPaginaAStorage(char* nombreFile, char* tag, int numeroPagina); // Hecho (a revisar)

int escucharStorageConfirmacion();

key_Reemplazo* ejecutarAlgoritmoReemplazo();

// Lectura/Escritura desde la "Memoria Interna"
char* leerContenidoDesdeOffset(char* nombreFile, char* tag, int numeroPagina, int numeroMarco, int offset, int size); //Falta
void escribirContenidoDesdeOffset(char* nombreFile, char* tag, int numeroPagina, int numeroMarco, char* contenido, int offset, int size); //Falta

void escribirEnMemoriaPaginaCompleta(char* nombreFile, char* tag, int numeroPagina, int marcoLibre, char* contenidoPagina, int size); //Falta
void* leerDesdeMemoriaPaginaCompleta(char* nombreFile, char* tag, int numeroMarco);

void asignarMarcoEntradaTabla(char* nombreFile, char* tag, int numeroPagina, int numeroMarco);
// int obtenerNumeroPaginaDeFileTag(const char* nombreFile, const char* tag, int direccionBase); //---No se si es necesario---


int64_t obtener_tiempo_actual();
void inicializar_entrada(EntradaDeTabla* entrada, int numeroPagina);
void actualizar_acceso_pagina(EntradaDeTabla* entrada);
void modificar_pagina(EntradaDeTabla* entrada); 



#endif