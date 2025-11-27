
#include "paquete.h"

t_paquete* crearPaquete() {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = 0;
    paquete->buffer->stream = NULL;
    return paquete;
}

void agregarOpcodeAPaquete(t_paquete* paquete, opcode codigo) {
    uint32_t nuevo_tamanio = paquete->buffer->size + sizeof(opcode);
    paquete->buffer->stream = realloc(paquete->buffer->stream, nuevo_tamanio);

    memcpy(paquete->buffer->stream + paquete->buffer->size, &codigo, sizeof(opcode));

    paquete->buffer->size = nuevo_tamanio;
}

void agregarStringAPaquete(t_paquete* paquete, char* string) {
    uint32_t tamanio_string = strlen(string) + 1;
    uint32_t nuevo_tamanio = paquete->buffer->size + sizeof(uint32_t) + tamanio_string;

    paquete->buffer->stream = realloc(paquete->buffer->stream, nuevo_tamanio);

    memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio_string, sizeof(uint32_t));
    memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(uint32_t), string, tamanio_string);

    paquete->buffer->size = nuevo_tamanio;
}

void agregarIntAPaquete(t_paquete* paquete, int valor) {
    uint32_t nuevo_tamanio = paquete->buffer->size + sizeof(int);
    paquete->buffer->stream = realloc(paquete->buffer->stream, nuevo_tamanio);

    memcpy(paquete->buffer->stream + paquete->buffer->size, &valor, sizeof(int));

    paquete->buffer->size = nuevo_tamanio;
}

void enviarPaquete(t_paquete* paquete, int socket) {
    send(socket, &(paquete->buffer->size), sizeof(uint32_t), 0);               
    send(socket, paquete->buffer->stream, paquete->buffer->size, 0);           
}

void eliminarPaquete(t_paquete* paquete) {
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

t_paquete* recibirPaquete(int socket_cliente) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    if (!paquete) return NULL;
    
    paquete->buffer = malloc(sizeof(t_buffer));
    if (!paquete->buffer) { free(paquete); return NULL; }

    ssize_t recibido = recv(socket_cliente, &(paquete->buffer->size), sizeof(uint32_t), MSG_WAITALL);
    if (recibido <= 0) { // 0 = cliente cerró, -1 = error
        free(paquete->buffer);
        free(paquete);
        return NULL;
    }


    paquete->buffer->stream = malloc(paquete->buffer->size);
    if (!paquete->buffer->stream && paquete->buffer->size > 0) {
        free(paquete->buffer);
        free(paquete);
        return NULL;
    }

    recibido = recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);
    if (recibido <= 0) { // 0 = cliente cerró, -1 = error
        free(paquete->buffer->stream);
        free(paquete->buffer);
        free(paquete);
        return NULL;
    }

    return paquete;
}

// ESTO POR AHORA ANDA, tanto RECIBIR y ENVIAR. Pero a grandes rasgos para cuando enviemos cosas serializadas lo tenemos que modificar
int recibirIntPaquete(t_paquete* paquete) {
    int valor;
    memcpy(&valor, paquete->buffer->stream, sizeof(int));
    return valor;
}


char* recibirStringDePaquete(t_paquete* paquete) {
    uint32_t longitud;
    int desplazamiento = 0;

    memcpy(&longitud, paquete->buffer->stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    char* str = malloc(longitud + 1); 
    memcpy(str, paquete->buffer->stream + desplazamiento, longitud);

    str[longitud] = '\0';

    return str; 
}

// Version con offset
char* recibirStringDePaqueteConOffset(t_paquete* paquete, int* offset) {
    if (!paquete) return NULL;
    uint32_t longitud;
    memcpy(&longitud, paquete->buffer->stream + *offset, sizeof(uint32_t));
    *offset += sizeof(uint32_t);

    char* str = malloc(longitud + 1);
    if (!str) return NULL;

    memcpy(str, paquete->buffer->stream + *offset, longitud);
    *offset += longitud;

    str[longitud] = '\0';
    return str;
}

int recibirIntDePaqueteconOffset(t_paquete* paquete, int* offset) {
    int valor;
    memcpy(&valor, paquete->buffer->stream + *offset, sizeof(int));
    *offset += sizeof(int);
    return valor;
}



void enviarOpcode(opcode codigo, int socket) {
    send(socket, &codigo, sizeof(opcode), 0);
}

opcode recibirOpcode(int socket) {
    opcode codigo;
    ssize_t recibido = recv(socket, &codigo, sizeof(opcode), MSG_WAITALL);
    if (recibido <= 0) {
        int err = errno;
        printf("recv socket %d devolvió %zd errno=%d (%s)\n",
                socket, recibido, err, strerror(err));
        return -1;
    }
    return codigo;
}
