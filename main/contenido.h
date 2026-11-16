#pragma once

#include <stdbool.h>

// Baja GET /resumen?formato=txt a la SD. Requiere Wi-Fi ya conectado.
// Devuelve: -1 error, 0 sin cambios en disco, 1 archivo actualizado.
int contenido_sincronizar(void);
