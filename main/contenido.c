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

bool contenido_sincronizar(void)
{
    char *buf = heap_caps_malloc(TAM_RESUMEN, MALLOC_CAP_SPIRAM);
    if (!buf) {
        return false;
    }
    int estado = cliente_get_texto("/resumen?formato=txt", buf, TAM_RESUMEN);
    bool ok = false;
    if (estado == 200 && buf[0]) {
        FILE *f = fopen(RUTA_RESUMEN, "wb");
        if (f) {
            fputs(buf, f);
            fclose(f);
            ok = true;
            ESP_LOGI(TAG, "Resumen guardado (%d bytes)", (int)strlen(buf));
        }
    } else {
        ESP_LOGW(TAG, "GET /resumen HTTP %d", estado);
    }
    free(buf);
    return ok;
}
