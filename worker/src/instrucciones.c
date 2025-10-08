#include "instrucciones.h"  


void ejecutar_create(char* fileName, char* tagFile){
    enviarOpcode(CREATE_FILE, socketStorage/*socket storage*/);

    t_paquete* paquete = crearPaquete();
    agregarStringAPaquete(paquete, fileName);
    agregarStringAPaquete(paquete, tagFile);
    
    enviarPaquete(paquete, socketStorage/*socket storage*/);

    eliminarPaquete(paquete);

}

void ejecutar_truncate(char* fileName, char* tagFile, int size){
    enviarOpcode(TRUNCATE_FILE, socketStorage/*socket storage*/);

    t_paquete* paquete = crearPaquete();
    agregarStringAPaquete(paquete, fileName);
    agregarStringAPaquete(paquete, tagFile);
    agregarIntAPaquete(paquete, size); 

    enviarPaquete(paquete, socketStorage/*socket storage*/);
    eliminarPaquete(paquete);

}

void ejecutar_write(char* fileName, char* tagFile, int direccionBase, char* contenido){
    int numeroPagina = pedirPagina(fileName, tagFile, direccionBase);
    if (numeroPagina == -1){
        log_error(logger, "Error al obtener o pedir pagina para WRITE en %s:%s direccionBase %d", fileName, tagFile, direccionBase);
        return;
    }

    int tamBloque = configW->BLOCK_SIZE;
    int Offset = direccionBase % tamBloque;

    int tamContenido = (int)strlen(contenido);                // strlen ok: decís que siempre tiene '\0'
    if (tamContenido > (tamBloque - Offset)) {
        log_error(logger, "WRITE: el contenido (%d bytes) no entra en la pagina desde offset %d (espacio %d)",
                  tamContenido, Offset, tamBloque - Offset);
        return;
    }

    for (int j = 0; j < tamContenido; j++) {
        escribirEnMemoriaByte(fileName, tagFile, numeroPagina, Offset + j, contenido[j]);
    }
    

}   

void ejecutar_read(char* fileName, char* tagFile, int direccionBase, int size, contexto_query_t* contexto){
    
}

void ejecutar_tag(char* fileNameOrigen, char* tagOrigen, char* fileNameDestino, char* tagDestino){
    enviarOpcode(TAG_FILE, socketStorage/*socket storage*/);    

    t_paquete* paquete = crearPaquete();
    agregarStringAPaquete(paquete, fileNameOrigen);
    agregarStringAPaquete(paquete, tagOrigen);
    agregarStringAPaquete(paquete, fileNameDestino);
    agregarStringAPaquete(paquete, tagDestino); 

    enviarPaquete(paquete, socketStorage/*socket storage*/);
    eliminarPaquete(paquete);
}   

void ejecutar_commit(char* fileNam, char* tagFile){
    
}   

void ejecutar_flush(void){
    
}       

void ejecutar_delete(char* fileNam, char* tagFile){
    enviarOpcode(DELETE_FILE, socketStorage/*socket storage*/);

    t_paquete* paquete = crearPaquete();
    agregarStringAPaquete(paquete, fileNam);
    agregarStringAPaquete(paquete, tagFile);
    enviarPaquete(paquete, socketStorage/*socket storage*/);
    eliminarPaquete(paquete);
}   

void ejecutar_end(contexto_query_t* contexto){
    enviarOpcode(END_QUERY, socketMaster/*socket master*/);
    
    t_paquete* paquete = crearPaquete();
    agregarIntAPaquete(paquete, contexto->query_id);
    enviarPaquete(paquete, socketMaster/*socket master*/);
    eliminarPaquete(paquete);
    
}       

