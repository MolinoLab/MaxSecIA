#pragma once

#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef enum {
    EV_BOOT_CORTO,    // atrás / cancelar grabación
    EV_BOOT_LARGO,    // empieza a grabar (mantener)
    EV_BOOT_SUELTO,   // suelta tras pulsación larga: termina la grabación
    EV_PWR_CORTO,     // bloqueo de pantalla
    EV_PWR_LARGO,     // apagar
    EV_ARRIBA,
    EV_ABAJO,
    EV_SELECT,
} evento_boton_t;

void botones_iniciar(bool despertado_por_boot);
QueueHandle_t botones_cola(void);
