#include <memoria_interna.h>


char* inicializarMemoriaInterna(void) {
    int tam_memoria = configW->tamMemoria;
    char* memoria = malloc(tam_memoria +1); 

    if (memoria == NULL) {
        log_error(logger, "Error al asignar memoria interna de tamaño %d", tam_memoria);
        exit(EXIT_FAILURE);
    }

    return memoria;
}

void finalizarMemoriaInterna(char* memoria) {
    free(memoria);
}

int ReemplazoLRU() {
    // Implementación del algoritmo de reemplazo LRU
    return -1; // Devuelve la página reemplazada (ejemplo)
}

int ReemplazoCLOCKM() {
    // Implementación del algoritmo de reemplazo CLOCK-M
    return -1; // Devuelve la página reemplazada (ejemplo)
}
