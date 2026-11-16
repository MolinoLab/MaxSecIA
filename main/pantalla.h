#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "placa.h"

void pantalla_iniciar(void);

// Copia `EPD_BYTES` (1 = blanco, 0 = negro, MSB = píxel izquierdo) y refresca
// el panel entero. Vuelve enseguida; el refresco completo tarda unos segundos.
bool pantalla_mostrar(const uint8_t *fb);

// Igual, pero si el frame coincide con el último no toca el panel. Si solo
// cambian franjas verticales, refresca esas ventanas (sin reiniciar el controlador).
// Devuelve false si no programó ningún refresco.
bool pantalla_actualizar(const uint8_t *fb);

// Ventana del frame ya en RAM del controlador. x múltiplo de 8.
bool pantalla_refresco_parcial(int x, int y, int w, int h, const uint8_t *fb);

bool pantalla_ocupada(void);
