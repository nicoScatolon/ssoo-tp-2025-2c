#ifndef UTILS_SOCKETS_H
#define UTILS_SOCKETS_H

#include <stdint.h> 
#include <sys/socket.h>
#include <netdb.h>
#include <commons/log.h>


void liberarConexion(int socket);
void enviarHandshake(int socket,int modulo);
int enviarBuffer(void* buffer,uint32_t size,int socketCliente);
int iniciarServidor(char * puerto, t_log* logger, char* modulo);
int esperarCliente(int socket_servidor,t_log* logger);
void* recibirBuffer(uint32_t* size, int socket_cliente);
int crearConexion(char *ip, char* puerto,t_log * logger);
void comprobarSocket(int socket, char* moduloOrigen, char* moduloDestino,t_log*logger);
#endif