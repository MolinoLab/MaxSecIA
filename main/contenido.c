#include "contenido.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "almacen.h"
#include "cliente.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "contenido";

#define TAM_RESUMEN (24 * 1024)

int contenido_sincronizar(void)
{
    char *buf = heap_caps_malloc(TAM_RESUMEN, MALLOC_CAP_SPIRAM);
    if (!buf) {
        return -1;
    }
    int estado = cliente_get_texto("/resumen?formato=txt", buf, TAM_RESUMEN);
    int r = -1;
    if (estado == 200 && buf[0]) {
        bool igual = false;
        FILE *prev = fopen(RUTA_RESUMEN, "rb");
        if (prev) {
            char *viejo = heap_caps_malloc(TAM_RESUMEN, MALLOC_CAP_SPIRAM);
            if (viejo) {
                size_t n = fread(viejo, 1, TAM_RESUMEN - 1, prev);
                viejo[n] = '\0';
                igual = strcmp(viejo, buf) == 0;
                free(viejo);
            }
            fclose(prev);
        }
        if (igual) {
            ESP_LOGI(TAG, "Resumen sin cambios (%d bytes)", (int)strlen(buf));
            r = 0;
        } else {
            FILE *f = fopen(RUTA_RESUMEN, "wb");
            if (f) {
                fputs(buf, f);
                fclose(f);
                r = 1;
                ESP_LOGI(TAG, "Resumen guardado (%d bytes)", (int)strlen(buf));
            }
        }
    } else {
        ESP_LOGW(TAG, "GET /resumen HTTP %d", estado);
    }
    free(buf);
    return r;
}
