#ifndef ALGORITMOS_REEMPLAZO_H
#define ALGORITMOS_REEMPLAZO_H

#include "globals.h"

extern configWorker * configW;

extern t_dictionary* tablasDePaginas;

// Algoritmos de reemplazo (devuelven la página reemplazada)
char* ReemplazoLRU(EntradaDeTabla**); 
char*ReemplazoCLOCKM(EntradaDeTabla**);
char* limpiar_puntero_clock();
void limpiar_puntero_clockM();
bool buscar_victima_clock(t_list* keys, EntradaDeTabla** victima, char** keyOut, bool buscarBitUso, bool buscarBitMod, bool limpiarBitUso);
int encontrar_indice_fileTag(t_list* keys, char* keyBuscada);
// int contar_paginas_presentes(t_list* keys);


typedef struct {
    char* keyProceso;
    int indicePagina;
} PunteroClockModificado;


#endif