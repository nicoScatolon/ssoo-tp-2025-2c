#include "inicializar.h"
void inicializarListas() {
    READY = list_create();
    EXECUTE = list_create();
    EXIT = list_create();
    workers = list_create();
    queriesControl = list_create();
}