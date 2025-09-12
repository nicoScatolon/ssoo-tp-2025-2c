#include "utils/config.h"
#include "utils/logs.h"
#include "utils/paquete.h"
#include "globals.h"
#include "conexiones.h"


void agregarQueryControl(char* path,int socketCliente, int prioridad){

    queryControl *nuevaQueryControl = malloc(sizeof(queryControl));
    nuevaQueryControl->path = strdup(path);
    nuevaQueryControl->socket = socketCliente;
    nuevaQueryControl->prioridad = prioridad;
    nuevaQueryControl->queryControlID = generarIdQueryControl(); 
    pthread_mutex_init(&nuevaQueryControl->mutex,NULL);

    listaAdd(nuevaQueryControl,&listaQueriesControl);

    pthread_mutex_lock(&mutex_cantidadQueriesControl);
    cantidadQueriesControl++;
    pthread_mutex_unlock(&mutex_cantidadQueriesControl);

    log_info(logger, "## Se conecta un Query Control para ejecutar la Query <PATH_QUERY> <%s> con prioridad <PRIORIDAD> <%d> - Id asignado: <QUERY_ID> <%d> . Nivel multiprogramación <CANTIDAD> <%d>",path,prioridad,nuevaQueryControl->queryControlID,cantidadQueriesControl);
    agregarQuery(path,prioridad,nuevaQueryControl->queryControlID);
}
void agregarWorker(int socketCliente,int idWorker){

    worker *nuevoWorker = malloc(sizeof(worker));
    nuevoWorker->pathActual = strdup("");
    nuevoWorker->socket = socketCliente;
    nuevoWorker->ocupado = false;
    nuevoWorker->workerID = idWorker;
    pthread_mutex_init(&nuevoWorker->mutex,NULL);

    listaAdd(nuevoWorker,&listaWorkers);

    pthread_mutex_lock(&mutex_grado);
    gradoMultiprogramacion++;
    pthread_mutex_unlock(&mutex_grado);

    log_info(logger,"Conexión de Worker: ## Se conecta el Worker <WORKER_ID> <%d> - Cantidad total de Workers: <CANTIDAD> <%d> ",nuevoWorker->workerID,gradoMultiprogramacion);

}
void agregarQuery(char* path,int prioridad,int id){
    query *nuevaQuery = malloc(sizeof(query));
    nuevaQuery->path = strdup(path);
    qcb_t *nuevoqcb = malloc(sizeof(qcb_t));
    nuevaQuery->qcb= nuevoqcb;
    nuevaQuery->qcb->prioridad = prioridad;
    nuevaQuery->qcb->queryID = id;
    nuevaQuery->qcb->PC = 0;
    pthread_mutex_init(&nuevaQuery->mutex,NULL);

    listaAdd(nuevaQuery,&listaReady);
    
    log_debug(logger,"Se encolo query ID <%d> en READY",nuevaQuery->qcb->queryID);
}

void eliminarQueryControl(queryControl* queryC){
    pthread_mutex_destroy(&queryC->mutex);
    free(queryC->path);
    free(queryC);
}
void eliminarWorker(worker* workerEliminar){
    pthread_mutex_destroy(&workerEliminar->mutex);
    free(workerEliminar->pathActual);
    free(workerEliminar);
}
void eliminarQuery(query * queryEliminar){
    pthread_mutex_destroy(&queryEliminar->mutex);
    free(queryEliminar->path);
    free(queryEliminar->qcb);
    free(queryEliminar);
}
uint32_t generarIdQueryControl() {
    pthread_mutex_lock(&mutex_id_queryControl);
    uint32_t id = siguienteIdQueryControl++;
    pthread_mutex_unlock(&mutex_id_queryControl);
    return id;
}

void establecerConexiones(){
    char* puertoQueryControl = string_itoa(configM->puertoEscuchaQueryControl);
    char* puertoWorker = string_itoa(configM->puertoEscuchaWorker);
    
    int socketQueryControl = iniciarServidor(puertoQueryControl,logger,"MASTER_QUERYCONTROL");
    int socketWorker = iniciarServidor(puertoWorker,logger,"MASTER_WORKER");
    //int socketWorker= iniciarServidor(puertoWorker,logger,"MASTER");
    free(puertoWorker);
    free(puertoQueryControl);

    
    pthread_t hiloQueryControl;
    pthread_t hiloWorker;
    pthread_create(&hiloQueryControl,NULL,escucharQueryControl,(void*)(intptr_t)socketQueryControl);
    pthread_create(&hiloWorker,NULL,escucharWorker,(void*)(intptr_t)socketWorker);

    pthread_detach(hiloQueryControl);
    pthread_detach(hiloWorker);
}

void *escucharQueryControl(void* socketServidorVoid){
    int socketServidor = (intptr_t) socketServidorVoid;
    log_debug(logger,"Servidor MASTER_QUERY_CONTROL escuchando conexiones");
    while (1)
    {
        int socketCliente = esperarCliente(socketServidor,logger);
        printf("socketCliente %d",socketCliente);
        modulo moduloOrigen;
        recv(socketCliente,&moduloOrigen,sizeof(modulo),0);
        comprobacionModulo(moduloOrigen,QUERY_CONTROL,"QUERYCONTROL",operarQueryControl,socketCliente);
    }
    return NULL;
}
void *escucharWorker(void* socketServidorVoid){
    int socketServidor = (intptr_t) socketServidorVoid;
    log_debug(logger,"Servidor MASTER_WORKER escuchando conexiones");
    while (1)
    {
        int socketCliente = esperarCliente(socketServidor,logger);
        printf("socketCliente %d",socketCliente);
        modulo moduloOrigen;
        recv(socketCliente,&moduloOrigen,sizeof(modulo),0);
        comprobacionModulo(moduloOrigen,WORKER,"WORKER",operarWorker,socketCliente);
    }
    return NULL;
}
void comprobacionModulo(modulo modulo_origen, modulo esperado, char *modulo, void*(*operacion)(void*), int socket_cliente)
{
    if (modulo_origen == esperado)
    {
        log_debug(logger, "Se conecto %s", modulo);
        pthread_t hilo_operacion;
        //log_debug(logger,"Aca no se rompio");
        if (pthread_create(&hilo_operacion, NULL, operacion, (void*)(intptr_t)socket_cliente) != 0) {
            log_error(logger, "No se pudo crear el hilo para %d", esperado);
            close(socket_cliente);
        }
        //log_debug(logger,"Se rompio antes del detach");
        pthread_detach(hilo_operacion); // Operaciones de modulos
    }
    else
    {
        log_warning(logger, "No es %s", modulo);
        close(socket_cliente);
    }
}
void * operarQueryControl(void* socketClienteVoid){
    int socketCliente =(intptr_t) socketClienteVoid;
    //log_debug(logger,"Llego a operarQueryControl");
    while(1){
        opcode codigo = recibirOpcode(socketCliente);
        if (codigo < 0) {
            log_warning(logger, "Cliente desconectado en socket %d", socketCliente);
            close(socketCliente);
            return NULL;
        }
        switch (codigo)
        {
        case INICIAR_QUERY_CONTROL:{
            t_paquete* paquete = recibirPaquete(socketCliente);
            if (!paquete) {
                log_error(logger, "Error recibiendo paquete en socket %d", socketCliente);
                break;
            }

            int offset = 0;
            char* path = recibirStringDePaqueteConOffset(paquete, &offset);
            if (!path) {
                log_error(logger, "Error desempaquetando path");
                eliminarPaquete(paquete);
                break;
            }
            int prioridad = recibirIntDePaqueteconOffset(paquete, &offset);
            agregarQueryControl(path,socketCliente,prioridad);
            free(path);
            eliminarPaquete(paquete);
            break;
        }
        case DESCONEXION_QUERY_CONTROL:{
            
        }
        default:
            log_warning(logger, "Opcode desconocido: %d", codigo);
            return NULL;
            break;
        }
    }
    close(socketCliente);
    return NULL;
}
void *operarWorker(void*socketClienteVoid){
    int socketCliente = (intptr_t) socketClienteVoid;
    while(1){
        opcode codigo = recibirOpcode(socketCliente);
            if (codigo < 0) {
            log_warning(logger, "Cliente desconectado en socket %d", socketCliente);
            close(socketCliente);
            return NULL;
        }
    switch (codigo)
    {
    case INICIAR_WORKER:{
        t_paquete* paquete = recibirPaquete(socketCliente);
        if (!paquete) {
                log_error(logger, "Error recibiendo paquete en socket %d", socketCliente);
                break;
            }
            int offset = 0;
            int idWorker = recibirIntDePaqueteconOffset(paquete,&offset);
            agregarWorker(socketCliente,idWorker);
            eliminarPaquete(paquete);
    }
        break;
    
    default:
        break;
    }
    }
}