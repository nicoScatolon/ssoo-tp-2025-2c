#ifndef INSTRUCCIONES_H
#define INSTRUCCIONES_H






void ejecutar_create(const char* nombreFile, const char* tagFile);  
void ejecutar_truncate(const char* nombreFile, const char* tagFile, int size);
void ejecutar_write(const char* nombreFile, const char* tagFile, int direccionBase, const char* contenido);
void ejecutar_read(const char* nombreFile, const char* tagFile, int direccionBase, int size);
void ejecutar_tag(const char* nombreFileOrigen, const char* tagOrigen, const char* nombreFileDestino, const char* tagDestino);
void ejecutar_commit(const char* nombreFile, const char* tagFile);
void ejecutar_flush(void);
void ejecutar_delete(const char* nombreFile, const char* tagFile);
void ejecutar_end(void);    


#endif