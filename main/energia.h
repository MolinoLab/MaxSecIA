#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/i2c_master.h"

typedef enum {
    DESPERTAR_ENCENDIDO,  // arranque normal (botón PWR o USB)
    DESPERTAR_BOOT,       // botón BOOT durante el sueño profundo: grabar
    DESPERTAR_PWR,        // botón PWR durante el sueño profundo
    DESPERTAR_TEMPORIZADOR,
} despertar_t;

typedef enum {
    LED_APAGADO,
    LED_FIJO,
    LED_PARPADEO,
} modo_led_t;

// Lo primero en app_main: enclava la batería y libera los pines retenidos durante el sueño.
despertar_t energia_iniciar(void);

// AXP2101: enciende ALDO1/2/3 a 3,3 V (pantalla, SD, periféricos). Hay que
// llamarlo antes de montar la microSD.
void energia_pmic_iniciar(i2c_master_bus_handle_t bus);

// Activa el corte por mantener PWR. Llamar solo cuando ese botón ya se ha soltado.
void energia_armar_apagado(void);

void energia_audio(bool encendido);
void energia_pantalla(bool encendido);
void energia_led(modo_led_t modo);

// Tensión de la batería en mV (0 si no se puede medir).
int energia_bateria_mv(void);

// Duerme hasta pulsar un botón o hasta `segundos` (0 = sin temporizador). No vuelve.
void energia_dormir(uint32_t segundos);

// Corta la batería por el AXP2101. Con USB la placa sigue alimentada: entonces duerme sin temporizador.
void energia_apagar(void);
