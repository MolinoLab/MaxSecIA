#pragma once

#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef enum {
    EV_BOOT_CORTO,    // grabar / terminar (como MiniSecIA)
    EV_BOOT_LARGO,    // sincronizar
    EV_PWR_CORTO,     // cancelar grabación, o estado de la cola
    EV_PWR_LARGO,     // apagar
    EV_ARRIBA,
    EV_ABAJO,
    EV_SELECT_CORTO,  // entrar / abrir
    EV_SELECT_LARGO,  // salir / atrás
} evento_boton_t;

// Crea la tarea de lectura. Un botón que ya está pulsado al arrancar cuenta desde ese momento
// (BOOT que despierta del sueño), salvo PWR, que se ignora hasta soltarlo (es el de encendido).
// Con `despertado_por_boot` y BOOT ya suelto, se emite una pulsación corta.
void botones_iniciar(bool despertado_por_boot);

QueueHandle_t botones_cola(void);
