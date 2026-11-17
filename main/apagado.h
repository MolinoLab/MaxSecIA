#pragma once

#include <stdint.h>

// Frame del panel (800x480, 1 = blanco) elegido al azar entre las tres
// imágenes de apagado. El puntero apunta a flash y sigue siendo válido.
const uint8_t *apagado_imagen_aleatoria(void);
