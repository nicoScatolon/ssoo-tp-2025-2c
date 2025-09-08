#include "utils/config.h"
#include "utils/logs.h"
#include "conexiones.h"
#include "utils/globales.h"
#include "utils/paquete.h"
#include "globals.h"


void agregarQueryControl(char* path,int socketCliente, int prioridad){
    queryControl *nuevaQueryControl = malloc(sizeof(queryControl));
    nuevaQueryControl->path = strdup(path);
    nuevaQueryControl->socket = socketCliente;
    nuevaQueryControl->prioridad = prioridad;
    nuevaQueryControl->queryControlID = generarIdQueryControl(); 
    pthread_mutex_init(&nuevaQueryControl->mutex,NULL);
    list_add(queriesControl,nuevaQueryControl);
    pthread_mutex_lock(&mutex_cantidadQueriesControl);
    cantidadQueriesControl++;
    pthread_mutex_unlock(&mutex_cantidadQueriesControl);
    log_info(logger, "## Se conecta un Query Control para ejecutar la Query <PATH_QUERY> <%s> con prioridad <PRIORIDAD> <%d> - Id asignado: <QUERY_ID> <%d> . Nivel multiprogramación <CANTIDAD> <%d>",path,prioridad,nuevaQueryControl->queryControlID,cantidadQueriesControl);
    agregarQuery(path,prioridad,nuevaQueryControl->queryControlID);
}
void agregarWorker(int socketCliente){
    worker *nuevoWorker = malloc(sizeof(worker));
    nuevoWorker->pathActual = strdup("");
    nuevoWorker->socket = socketCliente;
    nuevoWorker->ocupado = false;
    nuevoWorker->workerID = generarIdWorker();
    pthread_mutex_init(&nuevoWorker->mutex,NULL);
    pthread_mutex_lock(&mutex_lista_workers);
    list_add(workers,nuevoWorker);
    pthread_mutex_unlock(&mutex_lista_workers);
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
    list_add(READY,nuevaQuery);
    log_debug(logger,"Se encolo query ID <%d> en READY",nuevaQuery->qcb->queryID);
}

uint32_t generarIdQueryControl() {
    pthread_mutex_lock(&mutex_id_queryControl);
    uint32_t id = siguienteIdQueryControl++;
    pthread_mutex_unlock(&mutex_id_queryControl);
    return id;
}
uint32_t generarIdWorker() {
    pthread_mutex_lock(&mutex_id_worker);
    uint32_t id = siguienteIdWorker++;
    pthread_mutex_unlock(&mutex_id_worker);
    return id;
}

void establecerConexiones(){
    char* puertoQueryControl = string_itoa(configM->puertoEscucha);
    int socketQueryControl= iniciarServidor(puertoQueryControl,logger,"MASTER");
    free(puertoQueryControl);
    
    pthread_t hiloQueryControl;
    pthread_create(&hiloQueryControl,NULL,escucharQueryControl,(void*)(intptr_t)socketQueryControl);
    pthread_detach(hiloQueryControl);
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
            log_debug(logger,"antes de recibir el string");
            
            char* path = recibirStringDePaqueteConOffset(paquete, &offset);
            if (!path) {
                log_error(logger, "Error desempaquetando path");
                eliminarPaquete(paquete);
                break;
            }

            log_debug(logger,"despues de recibir el string, valor: %s",path);
            int prioridad = recibirIntDePaqueteconOffset(paquete, &offset);
            
            log_debug(logger,"prioridad %d, path %s ",prioridad,path);

            agregarQueryControl(path,socketCliente,prioridad);
            
            free(path);
            eliminarPaquete(paquete);
            break;
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