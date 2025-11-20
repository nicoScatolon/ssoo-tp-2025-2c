#include "instrucciones.h"  


void ejecutar_create(char* fileName, char* tagFile, int query_id){
    enviarOpcode(CREATE_FILE, socketStorage);

    t_paquete* paquete = crearPaquete();
    agregarIntAPaquete(paquete, query_id);
    agregarStringAPaquete(paquete, fileName);
    agregarStringAPaquete(paquete, tagFile);
    
    enviarPaquete(paquete, socketStorage);

    int respuesta = escucharStorage();
    if (respuesta == -1){
        notificarMasterError();
    }
    eliminarPaquete(paquete);
}

// Esperar confirmacion de storage luego de hacer un truncate antes de ejecutar la siguiente instruccion.
void ejecutar_truncate(char* fileName, char* tagFile, int size, int query_id){
    enviarOpcode(TRUNCATE_FILE, socketStorage);

    t_paquete* paquete = crearPaquete();
    agregarIntAPaquete(paquete, query_id);
    agregarStringAPaquete(paquete, fileName);
    agregarStringAPaquete(paquete, tagFile);
    agregarIntAPaquete(paquete, size); 

    enviarPaquete(paquete, socketStorage);
    eliminarPaquete(paquete);

    int respuesta = escucharStorage();
    if (respuesta == -1){
        notificarMasterError();
    }
}

void ejecutar_write(char* fileName, char* tagFile, int direccionBase, char* contenido, contexto_query_t* contexto){
    
    int offsetInicial = calcularOffsetDesdeDireccionBase(direccionBase);
    int primerPagina = calcularPaginaDesdeDireccionBase(direccionBase);
    int tamanioContenido = strlen(contenido);
    
    // Calcular última página que necesitamos
    int ultimaPagina = (direccionBase + tamanioContenido - 1) / configW->BLOCK_SIZE;
    
    int offsetDentroContenido = 0;

    for (int paginaActual = primerPagina; paginaActual <= ultimaPagina; paginaActual++) {
        int offsetEnPagina = (paginaActual == primerPagina) ? offsetInicial : 0;
        
        int bytesRestantesEnContenido = tamanioContenido - offsetDentroContenido;
        int espacioDisponibleEnPagina = configW->BLOCK_SIZE - offsetEnPagina;
        int bytesAEscribir = (bytesRestantesEnContenido < espacioDisponibleEnPagina) 
                             ? bytesRestantesEnContenido 
                             : espacioDisponibleEnPagina;
        
        int marco = obtenerNumeroDeMarco(fileName, tagFile, paginaActual);
        if (marco == -1) {
            log_error(logger, "Error al obtener marco para WRITE");
            notificarMasterError();
            return;
        }
        
        char* contenidoPagina = string_substring(contenido, offsetDentroContenido, bytesAEscribir);
        if (!contenidoPagina) {
            log_error(logger, "Error al extraer substring");
            exit(EXIT_FAILURE);
            return;
        }
        
        escribirContenidoDesdeOffset(fileName, tagFile, paginaActual, marco, 
                                      contenidoPagina, offsetEnPagina, bytesAEscribir);
        
        log_info(logger, "Query <%d>: Accion: <ESCRIBIR> - Direccion Fisica: <%d %d> - Valor: <%s>", 
                 contexto->query_id, marco, offsetEnPagina, contenidoPagina);
        
        free(contenidoPagina);
        offsetDentroContenido += bytesAEscribir;
    }
}

void ejecutar_read(char* fileName, char* tagFile, int direccionBase, int size, contexto_query_t* contexto){
    int offsetInicial = calcularOffsetDesdeDireccionBase(direccionBase);
    int primerPagina = calcularPaginaDesdeDireccionBase(direccionBase);
    
    // Calcular última página que necesitamos
    int ultimaPagina = (direccionBase + size - 1) / configW->BLOCK_SIZE;
    
    int offsetDentroContenido = 0;
    char* contenidoCompleto = malloc(size + 1); // +1 para el null terminator
    if (!contenidoCompleto) {
        log_error(logger, "Error al asignar memoria para READ");
        notificarMasterError();
        return;
    }
    contenidoCompleto[0] = '\0';

    for (int paginaActual = primerPagina; paginaActual <= ultimaPagina; paginaActual++) {
        int offsetEnPagina = (paginaActual == primerPagina) ? offsetInicial : 0;
        
        int bytesRestantesALeer = size - offsetDentroContenido;
        int espacioDisponibleEnPagina = configW->BLOCK_SIZE - offsetEnPagina;
        int bytesALeer = (bytesRestantesALeer < espacioDisponibleEnPagina) 
                         ? bytesRestantesALeer 
                         : espacioDisponibleEnPagina;
        
        int marco = obtenerNumeroDeMarco(fileName, tagFile, paginaActual);
        if (marco == -1) {
            log_error(logger, "Error al obtener marco para READ en %s:%s direccionBase %d", 
                      fileName, tagFile, direccionBase);
            free(contenidoCompleto);
            notificarMasterError();
            return;
        }
        
        char* contenidoPagina = leerContenidoDesdeOffset(fileName, tagFile, paginaActual, 
                                                          marco, offsetEnPagina, bytesALeer);
        if (!contenidoPagina) {
            log_error(logger, "Error al leer contenido de página");
            free(contenidoCompleto);
            notificarMasterError();
            return;
        }
        
        // Concatenar el contenido leído
        strcat(contenidoCompleto, contenidoPagina);
        
        log_info(logger, "Query <%d>: Acción: <LEER> - Dirección Física: <%d %d> - Valor: <%s>", 
                 contexto->query_id, marco, offsetEnPagina, contenidoPagina);
        
        free(contenidoPagina);
        offsetDentroContenido += bytesALeer;
    }

    // Enviar el contenido completo al master
    enviarOpcode(READ_BLOCK, socketMaster);
    t_paquete* paquete = crearPaquete();
    agregarIntAPaquete(paquete, contexto->query_id);
    agregarStringAPaquete(paquete, fileName);
    agregarStringAPaquete(paquete, tagFile);
    agregarStringAPaquete(paquete, contenidoCompleto);
    
    enviarPaquete(paquete, socketMaster);

    eliminarPaquete(paquete);
    free(contenidoCompleto);
}

// void ejecutar_read(char* fileName, char* tagFile, int direccionBase, int size, contexto_query_t* contexto){
//     int numeroPagina = calcularPaginaDesdeDireccionBase(direccionBase);
//     int offset = calcularOffsetDesdeDireccionBase(direccionBase);

//     int marco = obtenerNumeroDeMarco(fileName, tagFile, numeroPagina);

//     if (marco == -1){
//         log_error(logger, "Error al obtener o pedir pagina para READ en %s:%s direccionBase %d", fileName, tagFile, direccionBase);
//         notificarMasterError();
//         return;
//     }

//     char* contenidoLeido = leerContenidoDesdeOffset(fileName, tagFile, numeroPagina, marco, offset, size);

//     enviarOpcode(READ_BLOCK, socketMaster/*socket master*/);
//     t_paquete* paquete = crearPaquete();
//     agregarIntAPaquete(paquete, contexto->query_id);
//     agregarStringAPaquete(paquete, fileName);
//     agregarStringAPaquete(paquete, tagFile);
//     agregarStringAPaquete(paquete, contenidoLeido);
    
//     enviarPaquete(paquete, socketMaster/*socket master*/);

//     log_info(logger, "Query <%d>: Acción: <LEER> - Dirección Física: <%d %d> - Valor: <%s>", contexto->query_id, marco, offset, contenidoLeido);

//     eliminarPaquete(paquete);
//     free(contenidoLeido);
    
// }

void ejecutar_tag(char* fileNameOrigen, char* tagOrigen, char* fileNameDestino, char* tagDestino, int query_id){
    enviarOpcode(TAG_FILE, socketStorage/*socket storage*/);    

    t_paquete* paquete = crearPaquete();
    agregarIntAPaquete(paquete, query_id);
    agregarStringAPaquete(paquete, fileNameOrigen);
    agregarStringAPaquete(paquete, tagOrigen);
    agregarStringAPaquete(paquete, fileNameDestino);
    agregarStringAPaquete(paquete, tagDestino); 

    enviarPaquete(paquete, socketStorage/*socket storage*/);
    eliminarPaquete(paquete);
}   

void ejecutar_commit(char* fileName, char* tagFile, int query_id){
    enviarOpcode(COMMIT_FILE, socketStorage/*socket storage*/);    

    t_paquete* paquete = crearPaquete();
    agregarIntAPaquete(paquete, query_id);
    agregarStringAPaquete(paquete, fileName);
    agregarStringAPaquete(paquete, tagFile);
    enviarPaquete(paquete, socketStorage/*socket storage*/);
    eliminarPaquete(paquete);
    
}   

void ejecutar_flush(char* fileName, char* tagFile, int query_id){
    TablaDePaginas* tabla = obtenerTablaPorFileYTag(fileName, tagFile);
    bool modificadas;
    if (tabla){
        modificadas = tabla->hayPaginasModificadas;
    }
    else{
        log_warning(logger, "No se encontró la tabla para %s:%s", fileName, tagFile);
        return;
    }

    if(modificadas){
        t_paquete* paquete = crearPaquete();
        agregarIntAPaquete(paquete, query_id);
        agregarStringAPaquete(paquete, fileName);
        agregarStringAPaquete(paquete, tagFile); 
        for (int i = 0; i < tabla->capacidadEntradas; i++){
            if(tabla->entradas[i].bitModificado){
                int nroPagina = tabla->entradas[i].numeroPagina;
                int nroMarco = tabla->entradas[i].numeroMarco;

                char* contenidoPagina = obtenerContenidoDelMarco(nroMarco, 0, configW->BLOCK_SIZE); // antes se llamaba a leerDesdeMemoriaPaginaCompleta pero creo q esta bien asi
                if (!contenidoPagina){
                    log_error(logger, "Error al obtener el contenido del marco %d para hacer FLUSH de %s:%s pagina %d", nroMarco, fileName, tagFile, nroPagina);
                    continue;
                }

                agregarIntAPaquete(paquete, nroPagina);
                agregarStringAPaquete(paquete, contenidoPagina); //ver como mandar el bloque entero
                free(contenidoPagina);
                
                tabla->entradas[i].bitModificado = false; //reseteo el bit modificado

                //SUMAR RETARDO Acceso a TP
            }
        }
        enviarOpcode(FLUSH_FILE, socketStorage/*socket storage*/);  
        enviarPaquete(paquete, socketStorage/*socket storage*/);
        eliminarPaquete(paquete);
        tabla->hayPaginasModificadas = false;
    }
    else{
        log_debug(logger, "No hay páginas modificadas para hacer FLUSH en %s:%s", fileName, tagFile);
    }

    free(tabla);   
    return;
}       

void ejecutar_delete(char* fileNam, char* tagFile, int query_id){
    enviarOpcode(DELETE_FILE, socketStorage/*socket storage*/);

    t_paquete* paquete = crearPaquete();
    agregarIntAPaquete(paquete, query_id);
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

//funcion  para todos las querys que llamen a escucharStorage(), se debe loggear el error (con el tipo de error) y finalizar la query
void notificarMasterError(){
    enviarOpcode(FINALIZACION_QUERY, socketMaster);

    t_paquete* paqueteAMaster = crearPaquete();
    if(!paqueteAMaster){
        log_error(logger, "Error recibiendo paquete de RESPUESTA_ERROR");
        return;
    }

    agregarIntAPaquete(paqueteAMaster, contexto->query_id);
    enviarPaquete(paqueteAMaster, socketMaster);
    eliminarPaquete(paqueteAMaster);
    sem_post(&sem_hayInterrupcion);
}