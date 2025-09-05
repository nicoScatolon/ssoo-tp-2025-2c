#include "logs.h"

t_log* iniciar_logger(char* modulo, t_log_level nivel) {
    char* log_ext = ".log";
    char* result = malloc(strlen(modulo) + strlen(log_ext) + 1);
    strcpy(result, modulo);

    if (!string_ends_with(modulo, log_ext))
        string_append(&result, log_ext);

    t_log* nuevo_logger = log_create(result, modulo, true, nivel);
    free(result);
    return nuevo_logger;
}