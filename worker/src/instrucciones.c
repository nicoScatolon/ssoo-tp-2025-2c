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

}

void ejecutar_write(char* fileName, char* tagFile, int direccionBase, char* contenido){
    
}   

void ejecutar_read(char* fileName, char* tagFile, int direccionBase, int size, contexto_query_t* contexto){
    
}

void ejecutar_tag(char* fileNameOrigen, char* tagOrigen, char* fileNameDestino, char* tagDestino){
    
}   

void ejecutar_commit(char* fileNam, char* tagFile){
    
}   

void ejecutar_flush(void){
    
}       

void ejecutar_delete(char* fileNam, char* tagFile){
    
}   

void ejecutar_end(contexto_query_t* contexto){
    
}       

