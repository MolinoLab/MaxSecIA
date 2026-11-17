#pragma once

#include <stddef.h>
#include <stdint.h>

typedef enum {
    SUBIDA_OK,              // 200, X-Voz-Estado: ok
    SUBIDA_PENDIENTE,       // 200, X-Voz-Estado: pendiente (guardado en pendientes.csv)
    SUBIDA_VACIA,           // 422 con X-Voz-Estado: vacio (sin voz); se borra
    SUBIDA_RECHAZADA,       // 413: demasiado grande; a /sdcard/fallidos
    SUBIDA_ERROR_CONFIG,    // 400/401/403: token o device_id mal configurados; se para la cola
    SUBIDA_REINTENTAR,      // red, 409, 5xx: se queda en la cola
} resultado_subida_t;

// POST /voz en multipart. `grabado_en` en ISO UTC o "" si el reloj no tenía hora.
resultado_subida_t cliente_subir(const char *ruta, const char *request_id, const char *grabado_en);

// GET de `camino` (p. ej. "/resumen?formato=txt"). Devuelve el código HTTP
// (o -1) y deja el cuerpo, terminado en '\0', en `buf`.
int cliente_get_texto(const char *camino, char *buf, size_t n);

// POST application/x-www-form-urlencoded. Devuelve el código HTTP, o -1.
int cliente_post_form(const char *camino, const char *cuerpo);
