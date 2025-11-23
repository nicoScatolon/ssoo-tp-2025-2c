#ifndef ALGORITMOS_REEMPLAZO_H
#define ALGORITMOS_REEMPLAZO_H

#include "globals.h"

extern configWorker * configW;

extern t_dictionary* tablasDePaginas;


extern pthread_mutex_t tabla_paginas_mutex;


typedef struct {
    char* keyProceso;
    int indiceMarco;
} PunteroClockModificado;


typedef struct {
    char* key;
    int pagina;
    int marco;
} key_Reemplazo;

// Algoritmos de reemplazo (devuelven la página reemplazada)

key_Reemplazo* ReemplazoLRU(); 
key_Reemplazo*ReemplazoCLOCKM();
char* limpiar_puntero_clock();
void limpiar_puntero_clockM();
bool buscar_victima_clock(t_list* keys, EntradaDeTabla** victima, char** keyOut, bool buscarBitUso, bool buscarBitMod, bool limpiarBitUso);
int encontrar_indice_fileTag(t_list* keys, char* keyBuscada);
// int contar_paginas_presentes(t_list* keys);

#endif