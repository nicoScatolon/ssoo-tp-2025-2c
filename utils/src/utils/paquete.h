#ifndef PACKET_H
#define PACKET_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h> //PRUEBO
#include "utils/globales.h"


typedef struct {
    uint32_t size;
    void* stream;
} t_buffer;

typedef struct {
    opcode opcode;
    t_buffer* buffer;
} t_paquete;


t_paquete* crearPaquete();
// void agregarOpcodeAPaquete(t_paquete* paquete, opcode codigo);
void agregarStringAPaquete(t_paquete* paquete, char* string); 
void agregarIntAPaquete(t_paquete* paquete, int valor);
void enviarPaquete(t_paquete* paquete, int socket);
void eliminarPaquete(t_paquete* paquete);
t_paquete* recibirPaquete(int socket_cliente);
int recibirIntPaquete(t_paquete* paquete);
char* recibirStringDePaquete(t_paquete* paquete);
char* recibirStringDePaqueteConOffset(t_paquete* paquete, int* offset);
opcode recibirOpcode(int socket);
void enviarOpcode(opcode codigo, int socket);
#endif 
