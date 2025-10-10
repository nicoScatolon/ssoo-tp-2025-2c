#include <globals.h>

// Declare or extern configW if it's defined elsewhere
extern configWorker* configW;

int calcularPaginaDesdeDireccionBase(int direccionBase){
    int tamBloque = configW->BLOCK_SIZE;
    return direccionBase / tamBloque;
}   

int calcularOffsetDesdeDireccionBase(int direccionBase){
    int tamBloque = configW->BLOCK_SIZE;
    return direccionBase % tamBloque;
}

