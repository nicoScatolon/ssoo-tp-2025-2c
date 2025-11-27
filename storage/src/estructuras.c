#include "estructuras.h"


void vaciarMemoria(void){

}






// -----------------------------------------------------------------------------
// Funciones para el manejo del bitmap
// -----------------------------------------------------------------------------


// liberar un bloque 
// void bitmapLiberarBloque(unsigned int index) {
//     if (!bitmap) return;
//     if ((size_t)index >= bitmap_num_bits) return;
//     bitarray_clean_bit(bitmap, (off_t)index);
// }

// bool bitmapBloqueOcupado(unsigned int index) {
//     if (!bitmap) return false;
//     if ((size_t)index >= bitmap_num_bits) return false;
//     return bitarray_test_bit(bitmap, (off_t)index);
// }

// // reservar el primer bloque libre, retorna su indice o -1 si no hay libres
// ssize_t bitmapReservarLibre(void) {
//     if (!bitmap) return -1;
//     for (off_t i = 0; i < (off_t)bitmap_num_bits; ++i) {
//         if (!bitarray_test_bit(bitmap, i)) {
//             bitarray_set_bit(bitmap, i);
//             return (ssize_t)i;
//         }
//     }
//     return -1;
// }

// destruirBitmap: sincroniza y libera recursos del bitmap (llamar al terminar)
