#include "instrucciones.h"  


void ejecutar_create(char* fileName, char* tagFile){
    enviarOpcode(CREATE_FILE, socketStorage/*socket storage*/);

    t_paquete* paquete = crearPaquete();
    agregarStringAPaquete(paquete, fileName);
    agregarStringAPaquete(paquete, tagFile);
    
    enviarPaquete(paquete, socketStorage/*socket storage*/);
    escucharStorage();
    eliminarPaquete(paquete);

}

// Esperar confirmacion de storage luego de hacer un truncate antes de ejecutar la siguiente instruccion.
void ejecutar_truncate(char* fileName, char* tagFile, int size){
    enviarOpcode(TRUNCATE_FILE, socketStorage/*socket storage*/);

    t_paquete* paquete = crearPaquete();
    agregarStringAPaquete(paquete, fileName);
    agregarStringAPaquete(paquete, tagFile);
    agregarIntAPaquete(paquete, size); 

    enviarPaquete(paquete, socketStorage/*socket storage*/);
    escucharStorage();
    eliminarPaquete(paquete);

}

void ejecutar_write(char* fileName, char* tagFile, int direccionBase, char* contenido, contexto_query_t* contexto){
    int numeroPagina = calcularPaginaDesdeDireccionBase(direccionBase);
    int offset = calcularOffsetDesdeDireccionBase(direccionBase);

    int marco = obtenerNumeroDeMarco(fileName, tagFile, numeroPagina);
    if (marco == -1){
        log_error(logger, "Error al obtener o pedir pagina para WRITE en %s:%s direccionBase %d", fileName, tagFile, direccionBase);
        // Hola soy Franco! Para mi el envio de finalizacion de query hacia el Master deberia ser aca!, ya que 
        // obtener Marco devuelve -1, entonoces aca rompe con el flujo de la ejecuccion de write.
        return;
    }

    escribirContenidoDesdeOffset(fileName, tagFile, numeroPagina, marco,  contenido, offset, strlen(contenido)); 

    log_info(logger, "Query <%d>: Acción: <ESCRIBIR> - Dirección Física: <%d %d> - Valor: <%s>", contexto->query_id, marco, offset, contenido);
    

}   

void ejecutar_read(char* fileName, char* tagFile, int direccionBase, int size, contexto_query_t* contexto){
    int numeroPagina = calcularPaginaDesdeDireccionBase(direccionBase);
    int offset = calcularOffsetDesdeDireccionBase(direccionBase);

    int marco = obtenerNumeroDeMarco(fileName, tagFile, numeroPagina);

    if (marco == -1){
        log_error(logger, "Error al obtener o pedir pagina para READ en %s:%s direccionBase %d", fileName, tagFile, direccionBase);
        return;
    }

    char* contenidoLeido = leerContenidoDesdeOffset(fileName, tagFile, numeroPagina, marco, offset, size);

    enviarOpcode(READ_BLOCK, socketMaster/*socket master*/);
    t_paquete* paquete = crearPaquete();
    agregarIntAPaquete(paquete, contexto->query_id);
    agregarStringAPaquete(paquete, fileName);
    agregarStringAPaquete(paquete, tagFile);
    agregarStringAPaquete(paquete, contenidoLeido);
    
    enviarPaquete(paquete, socketMaster/*socket master*/);

    log_info(logger, "Query <%d>: Acción: <LEER> - Dirección Física: <%d %d> - Valor: <%s>", contexto->query_id, marco, offset, contenidoLeido);

    eliminarPaquete(paquete);
    free(contenidoLeido);
    
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

void ejecutar_commit(char* fileName, char* tagFile){
    enviarOpcode(COMMIT_FILE, socketStorage/*socket storage*/);    

    t_paquete* paquete = crearPaquete();
    agregarStringAPaquete(paquete, fileName);
    agregarStringAPaquete(paquete, tagFile); 
    enviarPaquete(paquete, socketStorage/*socket storage*/);
    eliminarPaquete(paquete);
    
}   

void ejecutar_flush(char* fileName, char* tag){
    TablaDePaginas* tabla = obtenerTablaPorFileYTag(fileName, tag);
    bool modificadas = false;
    if (tabla != NULL){
        modificadas = tabla->hayPaginasModificadas;
    }
    else{
        log_warning(logger, "No se encontró la tabla para %s:%s", fileName, tag);
        return;
    }

    if(modificadas){
        t_paquete* paquete = crearPaquete();
        agregarStringAPaquete(paquete, fileName);
        agregarStringAPaquete(paquete, tag); 
        for (int i = 0; i < tabla->cantidadEntradasUsadas; i++){
            if(tabla->entradas[i].bitModificado){
                int nroPagina = tabla->entradas[i].numeroPagina;
                int nroFrame = tabla->entradas[i].numeroFrame;
                char* contenidoPagina = leerDesdeMemoriaPaginaCompleta(fileName, tag, nroFrame);
                
                agregarIntAPaquete(paquete, nroPagina);
                agregarStringAPaquete(paquete, contenidoPagina); //ver como mandar el bloque entero
                free(contenidoPagina);
                
                tabla->entradas[i].bitModificado = false; //reseteo el bit modificado
            }
        }
        enviarOpcode(FLUSH_FILE, socketStorage/*socket storage*/);  
        enviarPaquete(paquete, socketStorage/*socket storage*/);
        eliminarPaquete(paquete);
        tabla->hayPaginasModificadas = false;
    }
    else{
        log_debug(logger, "No hay páginas modificadas para hacer FLUSH en %s:%s", fileName, tag);
    }

    free(tabla);   
    return;

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

