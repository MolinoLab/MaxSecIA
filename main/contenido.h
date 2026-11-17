#pragma once

#include <stdbool.h>

// Baja GET /resumen?formato=txt a la SD. Requiere Wi-Fi ya conectado.
// Devuelve: -1 error, 0 sin cambios en disco, 1 archivo actualizado.
int contenido_sincronizar(void);

// Anota completar/archivar en la SD. Se envía en la próxima sincronización.
bool contenido_encolar(const char *accion, const char *tipo, const char *id);

// Envía la cola. Devuelve cuántas quedan por reintentar, o -1 si no había archivo.
int contenido_enviar_cola(void);

// Quita esa ficha de resumen.txt para que no reaparezca antes del próximo GET.
bool contenido_olvidar(const char *tipo, const char *id);
