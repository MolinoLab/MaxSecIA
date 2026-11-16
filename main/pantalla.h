#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "placa.h"

void pantalla_iniciar(void);

// Copia `EPD_BYTES` (1 = blanco, 0 = negro, MSB = píxel izquierdo) y refresca.
// Vuelve enseguida; el refresco completo tarda unos segundos.
bool pantalla_mostrar(const uint8_t *fb);

// Reservado: el panel admite refresco parcial, aún no cableado.
bool pantalla_refresco_parcial(int x, int y, int w, int h, const uint8_t *fb);

bool pantalla_ocupada(void);
