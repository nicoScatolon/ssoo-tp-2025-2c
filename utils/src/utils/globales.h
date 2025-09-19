#ifndef GLOBALES_H
#define GLOBALES_H

typedef enum
{
    INICIAR_QUERY_CONTROL,
    INICIAR_WORKER,
    DESCONEXION_QUERY_CONTROL,
    DESCONEXION_WORKER,
    LECTURA_QUERY_CONTROL,
    FINALIZACION_QUERY,
    DESALOJO_QUERY_PLANIFICADOR,
    DESALOJO_QUERY_DESCONEXION,
    NUEVA_QUERY,
    //Instrucciones Storage:
    HANDSHAKE_STORAGE_WORKER,
    CREATE_FILE,
    TRUNCAR_ARCHIVO,
    TAG_FILE,
    COMMIT_TAG,
    WRITE_BLOCK,
    READ_BLOCK,
    REMOVE_TAG,
    
    RESPUESTA_OK,
    RESPUESTA_ERROR
}opcode;

typedef enum{
    MASTER,
    QUERY_CONTROL,
    WORKER,
    STORAGE
}modulo;

extern pthread_mutex_t mutex_cv_planif;
extern pthread_cond_t cv_planif;

#endif