#include "sockets.h"
#include <stdlib.h>
#include <unistd.h>    
#include <string.h>    
#include <stdio.h>     

void liberarConexion(int socket){
    close(socket);
}

void enviarHandshake(int socket,int modulo){
    if (send(socket,&modulo,sizeof(int),0) <=0)
    {
        perror("Error al enviar el handshake");
        exit(EXIT_FAILURE);
    }
}

int enviarBuffer(void* buffer,uints32_t size,int socketCliente){
    return send(socketCliente,buffer,size,0);
}

int iniciarServidor(char * puerto, t_log* logger, char* modulo)
{
	struct addrinfo hints, *server_info;
	int socket_servidor =-1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL,puerto,&hints,&server_info) != 0){
        perror("Error en getaddinfo");
        exit(EXIT_FAILURE);
    }
	// Creamos el socket de escucha del servidor
	socket_servidor = socket(server_info->ai_family,server_info->ai_socktype,server_info->ai_protocol);
	if (socket_servidor == -1)
	{
		perror("Error al crear el socket_servidor");
		exit (EXIT_FAILURE);
	}
    // Esto esta para poder reutilizar el puerto una vez que se haya finalizado la ejecuccion
    int activado = 1;
    setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado));

	if (bind(socket_servidor,server_info->ai_addr,server_info->ai_addrlen) == -1){
        perror("Error en bind");
        exit(EXIT_FAILURE);
    }
	freeaddrinfo(server_info);

	if(listen(socket_servidor,SOMAXCONN) != 0){
        perror("Error en listen");
        exit(EXIT_FAILURE);
    }
	log_debug(logger, "Servidor %s escuchando en el puerto%s",modulo,puerto);

	return socket_servidor;
}

int esperarCliente(int socket_servidor,t_log* logger)
{
    // Esto nos permite ser consciente de quien se conecto
    struct sockaddr_in direccionCliente;
    socklen_t tamDireccion = sizeof(struct sockaddr_in);

	int socket_cliente = accept(socket_servidor,(void*)&direccionCliente,&tamDireccion);
	if(socket_cliente == -1){
        perror("Error al aceptar el cliente");
        exit(EXIT_FAILURE);
    }
    log_debug(logger, "Se conecto un cliente !");
	return socket_cliente;
}
void* recibirBuffer(uint32_t* size, int socket_cliente)
{
	void * buffer;
	recv(socket_cliente, size, sizeof(uint32_t), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);
	return buffer;
}
int crearConexion(char *ip, char* puerto,t_log * logger)
{
	struct addrinfo hints,*server_info;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;

	if(getaddrinfo(ip, puerto, &hints, &server_info)!= 0){
        perror("Error en getaddinfo");
        return -1;
    }
	// Ahora vamos a crear el socket.
	int socket_cliente = socket(server_info->ai_family,server_info->ai_socktype,server_info->ai_protocol);
	if (socket_cliente == -1)
	{
		log_error(logger,"Error al crear el socket del cliente");
        freeaddrinfo(server_info);
		return -1;
	}
	// Ahora que tenemos el socket, vamos a conectarlo
	connect(socket_cliente,server_info->ai_addr,server_info->ai_addrlen);
	if (connect onnect(socket_cliente,server_info->ai_addr,server_info->ai_addrlen) == -1){
		log_error(logger,"Error al conectarse con el cliente");
		return -1;
	}
	freeaddrinfo(server_info);
	return socket_cliente;
}
