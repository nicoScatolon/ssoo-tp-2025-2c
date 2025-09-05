#ifndef GLOBALS_H
#define GLOBALS_H

typedef enum
{
    INICIAR_QUERY_CONTROL,
    FINALIZAR_QUERY_CONTROL,
    INFORMACION_QUERY_CONTROL

}opcode;
typedef enum{
    MASTER,
    QUERY_CONTROL,
    WORKER,
    STORAGE
}modulo;

#endif