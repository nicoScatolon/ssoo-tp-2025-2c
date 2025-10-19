#include "operaciones.h"

char* lecturaBloque(char* file, char* tag, int numeroBloque){
    char rutaCompleta[512];
    snprintf(rutaCompleta, sizeof(rutaCompleta), 
             "%s/pathfiles/%s/%s/logical_blocks/%d.dat", 
             configS->puntoMontaje, file, tag, numeroBloque);


    FILE* bloque = fopen(rutaCompleta, "r");
    if (bloque == NULL) {
        log_debug(logger,"No Existe el bloque logico numero <%d> asociado al file:tag <%s:%s>\n", numeroBloque,file,tag);
        return NULL;
    }
    char* contenido = (char*)malloc((configSB->BLOCK_SIZE + 1) * sizeof(char));
    size_t bytesLeidos = fread(contenido, sizeof(char), configSB->BLOCK_SIZE, bloque);
    contenido[bytesLeidos] = '\0';
    fclose(bloque);
    usleep(configS->retardoAccesoBloque*1000);
    log_debug(logger,"Contenido del bloque <%d> asociado al file:tag <%s:%s>: %s",numeroBloque,file,tag,contenido);
    return contenido;
}