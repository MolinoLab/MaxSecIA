#include "apagado.h"

#include "esp_random.h"

extern const uint8_t apagado_0_start[] asm("_binary_apagado_0_bin_start");
extern const uint8_t apagado_1_start[] asm("_binary_apagado_1_bin_start");
extern const uint8_t apagado_2_start[] asm("_binary_apagado_2_bin_start");

const uint8_t *apagado_imagen_aleatoria(void)
{
    static const uint8_t *const imgs[] = {apagado_0_start, apagado_1_start, apagado_2_start};
    return imgs[esp_random() % 3];
}
