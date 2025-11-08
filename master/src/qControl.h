#ifndef QCONTROL_H
#define QCONTROL_H

#include "globals.h"
#include "conexiones.h"

void eliminarQueryControl(queryControl* queryC);
int obtenerPosicionQCPorId(int idBuscado);
queryControl* buscarQueryControlPorId(int idQuery);
queryControl* buscarQueryControlPorSocket(int socket);

#endif