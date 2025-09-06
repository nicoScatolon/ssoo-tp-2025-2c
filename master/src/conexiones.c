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
    list_add(workers,nuevoWorker);
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
        pthread_create(&hilo_operacion, NULL, operacion, (void *)(intptr_t)socket_cliente);
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

    while(1){
        opcode codigo = recibirOpcode(socketCliente);
        switch (codigo)
        {
        case INICIAR_QUERY_CONTROL:{
               
            t_paquete*paquete =recibirPaquete(socketCliente);
            char* path = recibirStringDePaquete(paquete);
            int prioridad = recibirIntPaquete(paquete);
            agregarQueryControl(path,socketCliente,prioridad);
            free(path);
            break;
        }
        default:
            break;
        }
    }
}