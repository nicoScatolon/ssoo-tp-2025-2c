#include "instrucciones.h"  


bool ejecutar_create(char* fileName, char* tagFile, int query_id){
    enviarOpcode(CREATE_FILE, socketStorage);

    t_paquete* paquete = crearPaquete();
    agregarIntAPaquete(paquete, query_id);
    agregarStringAPaquete(paquete, fileName);
    agregarStringAPaquete(paquete, tagFile);
    
    enviarPaquete(paquete, socketStorage);

    int respuesta = escucharStorage();
    if (respuesta == -1){
        notificarMasterError("Error al ejecutar CREATE");
        return false;
    }
    eliminarPaquete(paquete);
    return true;
}

// Esperar confirmacion de storage luego de hacer un truncate antes de ejecutar la siguiente instruccion.
bool ejecutar_truncate(char* fileName, char* tagFile, int size, int query_id){
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
        notificarMasterError("Error al ejecutar TRUNCATE");
        return false;
    }
    return true;
}

bool ejecutar_write(char* fileName, char* tagFile, int direccionBase, char* contenido, contexto_query_t* contexto){
    
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
            notificarMasterError("Error al obtener marco para WRITE");
            return false;
        }
        
        char* contenidoPagina = string_substring(contenido, offsetDentroContenido, bytesAEscribir);
        if (!contenidoPagina) {
            log_error(logger, "Error al extraer substring");
            exit(EXIT_FAILURE);
            return false;
        }
        
        escribirContenidoDesdeOffset(fileName, tagFile, paginaActual, marco, 
                                      contenidoPagina, offsetEnPagina, bytesAEscribir);
        
        log_info(logger, "Query <%d>: Accion: <ESCRIBIR> - Direccion Fisica: <%d %d> - Valor: <%s>", 
                 contexto->query_id, marco, offsetEnPagina, contenidoPagina);
        
        free(contenidoPagina);
        offsetDentroContenido += bytesAEscribir;
    }

    return true;
}


bool ejecutar_read(char* fileName, char* tagFile, int direccionBase, int size, contexto_query_t* contexto){
    int offsetInicial = calcularOffsetDesdeDireccionBase(direccionBase);
    int primerPagina = calcularPaginaDesdeDireccionBase(direccionBase);
    int ultimaPagina = (direccionBase + size - 1) / configW->BLOCK_SIZE;
    
    // Usar string_new() para inicializar un string vacío
    char* contenidoCompleto = string_new();
    if (!contenidoCompleto) {
        log_error(logger, "Error al asignar memoria para READ");
        exit(EXIT_FAILURE);
    }
    
    int bytesLeidos = 0;
    
    for (int paginaActual = primerPagina; paginaActual <= ultimaPagina; paginaActual++) {
        int offsetEnPagina = (paginaActual == primerPagina) ? offsetInicial : 0;
        int bytesRestantes = size - bytesLeidos;
        int espacioEnPagina = configW->BLOCK_SIZE - offsetEnPagina;
        int bytesALeer = (bytesRestantes < espacioEnPagina) ? bytesRestantes : espacioEnPagina;
        
        int marco = obtenerNumeroDeMarco(fileName, tagFile, paginaActual);
        if (marco == -1) {
            log_error(logger, "Error al obtener marco para página %d", paginaActual);
            free(contenidoCompleto);
            notificarMasterError("Error al traer pagina de Storage");
            return false;
        }
        
        char* contenidoPagina = leerContenidoDesdeOffset(fileName, tagFile, paginaActual, 
                                                         marco, offsetEnPagina, bytesALeer);
        if (!contenidoPagina) {
            log_error(logger, "Error al leer contenido de página");
            free(contenidoCompleto);
            notificarMasterError("Error al leer contenido de página");
            return false;
        }
        
        // Usar string_append para concatenar (maneja la reasignación automáticamente)
        string_append(&contenidoCompleto, contenidoPagina);
        
        log_info(logger, "Query <%d>: Acción: <LEER> - Dirección Física: <%d %d> - Valor: <%s>", 
                 contexto->query_id, marco, offsetEnPagina, contenidoPagina);
        
        free(contenidoPagina);
        bytesLeidos += bytesALeer;
    }
    
    // Si necesitas recortar al tamaño exacto
    log_debug(logger, "Tamaño leído: %d, Tamaño solicitado: %d, contenido completo sin recortar: %s", string_length(contenidoCompleto), size, contenidoCompleto);
    if (string_length(contenidoCompleto) > size) {
        char* contenidoRecortado = string_substring(contenidoCompleto, 0, size);
        contenidoCompleto = contenidoRecortado;
        log_debug(logger, "Contenido recortado al tamaño solicitado: %s", contenidoCompleto);
        //free(contenidoCompleto);
    }
    
    // Enviar resultado
    enviarOpcode(LECTURA_QUERY_CONTROL, socketMaster);
    t_paquete* paquete = crearPaquete();
    agregarIntAPaquete(paquete, contexto->query_id);
    agregarStringAPaquete(paquete, fileName);
    agregarStringAPaquete(paquete, tagFile);
    agregarStringAPaquete(paquete, contenidoCompleto);
    
    enviarPaquete(paquete, socketMaster);
    eliminarPaquete(paquete);
    free(contenidoCompleto);
    return true;
}


bool ejecutar_tag(char* fileNameOrigen, char* tagOrigen, char* fileNameDestino, char* tagDestino, int query_id){
    enviarOpcode(TAG_FILE, socketStorage/*socket storage*/);    

    t_paquete* paquete = crearPaquete();
    agregarIntAPaquete(paquete, query_id);
    agregarStringAPaquete(paquete, fileNameOrigen);
    agregarStringAPaquete(paquete, tagOrigen);
    agregarStringAPaquete(paquete, fileNameDestino);
    agregarStringAPaquete(paquete, tagDestino); 

    enviarPaquete(paquete, socketStorage/*socket storage*/);
    eliminarPaquete(paquete);

    int resp = escucharStorage(); //esperar confirmacion de storage
    if (resp == -1)
    {
        notificarMasterError("Error al ejecutar TAG");
        return false;
    }
    return true;
}   

bool ejecutar_commit(char* fileName, char* tagFile, int query_id){
    enviarOpcode(COMMIT_FILE, socketStorage/*socket storage*/);    

    t_paquete* paquete = crearPaquete();
    agregarIntAPaquete(paquete, query_id);
    agregarStringAPaquete(paquete, fileName);
    agregarStringAPaquete(paquete, tagFile);
    enviarPaquete(paquete, socketStorage/*socket storage*/);
    eliminarPaquete(paquete);

    int resp = escucharStorage(); //esperar confirmacion de storage
    if (resp == -1)
    {
        notificarMasterError("Error al ejecutar COMMIT");
        return false;
    }
    
    return true;
}   

bool ejecutar_flush(char* fileName, char* tagFile, int query_id){
    log_debug(logger, "Iniciando FLUSH para %s:%s", fileName, tagFile);
    TablaDePaginas* tabla = obtenerTablaPorFileYTag(fileName, tagFile);
    bool modificadas;
    if (tabla){
        modificadas = tabla->hayPaginasModificadas;
    }
    else{
        log_warning(logger, "No se encontró la tabla para %s:%s", fileName, tagFile);
        return false;
    }
    
    if(modificadas){
        int contadorPaginasModificadas = 0;
        for (int i = 0; i < tabla->capacidadEntradas; i++){
            if(tabla->entradas[i].bitModificado && tabla->entradas[i].bitPresencia){
                contadorPaginasModificadas++;
            }
        }
        
        t_paquete* paqueteAviso = crearPaquete();
        
        
        agregarIntAPaquete(paqueteAviso, query_id);
        agregarStringAPaquete(paqueteAviso, fileName);
        agregarStringAPaquete(paqueteAviso, tagFile); 
        agregarIntAPaquete(paqueteAviso, contadorPaginasModificadas);
        
        enviarOpcode(FLUSH_FILE, socketStorage);  
        enviarPaquete(paqueteAviso, socketStorage);
        eliminarPaquete(paqueteAviso);
        
        
        for (int i = 0; i < tabla->capacidadEntradas; i++){
            
            if(tabla->entradas[i].bitModificado && tabla->entradas[i].bitPresencia){
                t_paquete* paqueteContenido = crearPaquete();


                int nroPagina = tabla->entradas[i].numeroPagina;
                int nroMarco = tabla->entradas[i].numeroMarco;
                char* contenidoPagina = obtenerContenidoDelMarco(nroMarco, 0, configW->BLOCK_SIZE);
                
                if (!contenidoPagina){
                    log_error(logger, "Error al obtener el contenido del marco %d para hacer FLUSH de %s:%s pagina %d", 
                              nroMarco, fileName, tagFile, nroPagina);
                    continue;
                }
                
                log_debug(logger, "contenidoPagina numero <%d> a mandar en FLUSH: <%s>", nroPagina, contenidoPagina);
                agregarIntAPaquete(paqueteContenido, nroPagina);
                agregarStringAPaquete(paqueteContenido, contenidoPagina);
                free(contenidoPagina);
                tabla->entradas[i].bitModificado = false;
                
                enviarPaquete(paqueteContenido, socketStorage);
                
                if(escucharStorage() == -1) {
                    eliminarPaquete(paqueteContenido);
                    notificarMasterError("Error al ejecutar FLUSH");
                    return false;
                }
                eliminarPaquete(paqueteContenido);
            }
        }        
        
        log_debug(logger, "FLUSH completado para %s:%s", fileName, tagFile);
        tabla->hayPaginasModificadas = false;
    }
    else{
        log_debug(logger, "No hay páginas modificadas para hacer FLUSH en %s:%s", fileName, tagFile);
    }
    return true;
}

bool ejecutar_delete(char* fileNam, char* tagFile, int query_id){
    enviarOpcode(DELETE_FILE, socketStorage/*socket storage*/);

    t_paquete* paquete = crearPaquete();
    agregarIntAPaquete(paquete, query_id);
    agregarStringAPaquete(paquete, fileNam);
    agregarStringAPaquete(paquete, tagFile);
    enviarPaquete(paquete, socketStorage/*socket storage*/);
    eliminarPaquete(paquete);
    return true;
}   

bool ejecutar_end(contexto_query_t* contexto){
    enviarOpcode(FINALIZACION_QUERY, socketMaster/*socket master*/);
    
    t_paquete* paquete = crearPaquete();
    agregarIntAPaquete(paquete, contexto->query_id);
    agregarStringAPaquete(paquete, "Finalizacion de ejecucion de la query");
    enviarPaquete(paquete, socketMaster/*socket master*/);
    eliminarPaquete(paquete);
    return true;
}       

//funcion  para todos las querys que llamen a escucharStorage(), se debe loggear el error (con el tipo de error) y finalizar la query
void notificarMasterError(char* mensajeError){
    enviarOpcode(FINALIZACION_QUERY, socketMaster);

    t_paquete* paqueteAMaster = crearPaquete();
    if(!paqueteAMaster){
        log_error(logger, "Error al crear paquete para notificar error al Master");
        return;
    }

    log_debug(logger, "Notificando al Master el error: %s", mensajeError);

    agregarIntAPaquete(paqueteAMaster, contexto->query_id);
    agregarStringAPaquete(paqueteAMaster, mensajeError);
    enviarPaquete(paqueteAMaster, socketMaster);
    eliminarPaquete(paqueteAMaster);
    sem_post(&sem_hayInterrupcion);
}