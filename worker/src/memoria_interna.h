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
TablaDePaginas* obtenerTablaPorFileYTag(char* nombreFile, char* tag); // Hecho

int obtenerMarcoLibre(void); // Hecho
void liberarMarco(int nro_marco); // Hecho
int obtenerMarcoDesdePagina(char* nombreFile, char* tag, int numeroPagina); // Hecho
char* obtenerContenidoDelMarco(int nro_marco, int offset, int size);

int obtenerNumeroDeMarco(char* nombreFile, char* tag, int numeroPagina); // Hecho
 
void agregarContenidoAMarco(int numeroMarco, char* contenido);

char* traerPaginaDeStorage(char* nombreFile, char* tag, int numeroPagina);

int enviarPaginaAStorage(char* nombreFile, char* tag, int numeroPagina); // Hecho (a revisar)

int escucharStorageConfirmacion();

int ejecutarAlgoritmoReemplazo();

// Lectura/Escritura desde la "Memoria Interna"
char* leerContenidoDesdeOffset(char* nombreFile, char* tag, int numeroPagina, int numeroMarco, int offset, int size); //Falta
void escribirContenidoDesdeOffset(char* nombreFile, char* tag, int numeroPagina, int numeroMarco, char* contenido, int offset, int size); //Falta

void escribirEnMemoriaPaginaCompleta(char* nombreFile, char* tag, int numeroPagina, int marcoLibre, char* contenidoPagina, int size); //Falta
void* leerDesdeMemoriaPaginaCompleta(char* nombreFile, char* tag, int numeroMarco);


// int obtenerNumeroPaginaDeFileTag(const char* nombreFile, const char* tag, int direccionBase); //---No se si es necesario---


int64_t obtener_tiempo_actual();
void inicializar_entrada(EntradaDeTabla* entrada, int numeroPagina);
void actualizar_acceso_pagina(EntradaDeTabla* entrada);
void modificar_pagina(EntradaDeTabla* entrada); 

// Algoritmos de reemplazo (devuelven la página reemplazada)
char* ReemplazoLRU(EntradaDeTabla**); //Por Hacer         puede usar obtenerPaginaLibre(), reservarPagina(), liberarPagina(), agregarPaginaAProceso()
char*ReemplazoCLOCKM(EntradaDeTabla**); //Por Hacer      puede usar obtenerPaginaLibre(), reservarPagina(), liberarPagina(), agregarPaginaAProceso()
char* limpiar_puntero_clock();
void limpiar_puntero_clockM();
bool buscar_victima_clock(t_list* keys, EntradaDeTabla** victima, char** keyOut, bool buscarBitUso, bool buscarBitMod, bool limpiarBitUso);
int encontrar_indice_proceso(t_list* keys, char* keyBuscada);
int contar_paginas_presentes(t_list* keys);



typedef struct {
    char* keyProceso;
    int indicePagina;
} PunteroClockModificado;

static PunteroClockModificado punteroClockMod = {NULL, 0};


#endif