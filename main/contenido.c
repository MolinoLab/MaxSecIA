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

static int contenido_actuar(const char *tipo, const char *id, const char *accion)
{
    char cuerpo[128];
    int n = snprintf(cuerpo, sizeof(cuerpo), "tipo=%s&id=%s&accion=%s", tipo ? tipo : "", id ? id : "",
                     accion ? accion : "");
    if (n <= 0 || n >= (int)sizeof(cuerpo)) {
        return -1;
    }
    int estado = cliente_post_form("/tareas/accion", cuerpo);
    ESP_LOGI(TAG, "accion %s %s %s -> HTTP %d", tipo, id, accion, estado);
    return estado;
}

bool contenido_encolar(const char *accion, const char *tipo, const char *id)
{
    if (!accion || !accion[0] || !tipo || !tipo[0] || !id || !id[0]) {
        return false;
    }
    FILE *f = fopen(RUTA_ACCIONES, "a");
    if (!f) {
        ESP_LOGW(TAG, "No se pudo anotar la accion");
        return false;
    }
    fprintf(f, "%s\t%s\t%s\n", accion, tipo, id);
    fclose(f);
    ESP_LOGI(TAG, "En cola: %s %s %s", accion, tipo, id);
    return true;
}

int contenido_enviar_cola(void)
{
    FILE *in = fopen(RUTA_ACCIONES, "rb");
    if (!in) {
        return -1;
    }
    char *buf = heap_caps_malloc(4096, MALLOC_CAP_SPIRAM);
    char *queda = heap_caps_malloc(4096, MALLOC_CAP_SPIRAM);
    if (!buf || !queda) {
        free(buf);
        free(queda);
        fclose(in);
        return -1;
    }
    size_t n = fread(buf, 1, 4095, in);
    fclose(in);
    buf[n] = '\0';
    size_t w = 0;
    int pendientes = 0;
    char *p = buf;
    while (*p) {
        char *nl = strchr(p, '\n');
        size_t len = nl ? (size_t)(nl - p) : strlen(p);
        if (len && p[len - 1] == '\r') {
            len--;
        }
        char linea[96];
        bool reintentar = false;
        if (len > 0 && len < sizeof(linea)) {
            memcpy(linea, p, len);
            linea[len] = '\0';
            char *accion = linea;
            char *tipo = strchr(accion, '\t');
            char *id = tipo ? strchr(tipo + 1, '\t') : NULL;
            if (tipo && id) {
                *tipo++ = '\0';
                *id++ = '\0';
                int estado = contenido_actuar(tipo, id, accion);
                // 200 aplicada, 404 ya no existe, 400 línea inválida. El resto se reintenta.
                reintentar = !(estado == 200 || estado == 404 || estado == 400);
            }
        }
        if (reintentar && w + len + 1 < 4096) {
            memcpy(queda + w, p, len);
            w += len;
            queda[w++] = '\n';
            pendientes++;
        }
        if (!nl) {
            break;
        }
        p = nl + 1;
    }
    if (pendientes == 0) {
        remove(RUTA_ACCIONES);
    } else {
        FILE *out = fopen(RUTA_ACCIONES, "wb");
        if (out) {
            fwrite(queda, 1, w, out);
            fclose(out);
        }
    }
    free(queda);
    free(buf);
    ESP_LOGI(TAG, "Acciones pendientes: %d", pendientes);
    return pendientes;
}

static bool linea_de_ficha(const char *p, size_t len, const char *tipo, const char *id)
{
    size_t nt = strlen(tipo);
    size_t ni = strlen(id);
    if (len <= nt || strncmp(p, tipo, nt) != 0 || p[nt] != '\t') {
        return false;
    }
    int campo = 0;
    const char *ini = p;
    for (size_t i = 0; i <= len; i++) {
        if (i == len || p[i] == '\t') {
            if (campo == 3) {
                size_t n = (size_t)((p + i) - ini);
                return n == ni && memcmp(ini, id, ni) == 0;
            }
            campo++;
            ini = p + i + 1;
        }
    }
    return false;
}

bool contenido_olvidar(const char *tipo, const char *id)
{
    if (!tipo || !tipo[0] || !id || !id[0]) {
        return false;
    }
    FILE *in = fopen(RUTA_RESUMEN, "rb");
    if (!in) {
        return false;
    }
    char *buf = heap_caps_malloc(TAM_RESUMEN, MALLOC_CAP_SPIRAM);
    char *out = heap_caps_malloc(TAM_RESUMEN, MALLOC_CAP_SPIRAM);
    if (!buf || !out) {
        free(buf);
        free(out);
        fclose(in);
        return false;
    }
    size_t n = fread(buf, 1, TAM_RESUMEN - 1, in);
    fclose(in);
    buf[n] = '\0';

    size_t w = 0;
    bool quito = false;
    char *p = buf;
    while (*p && w + 2 < TAM_RESUMEN) {
        char *nl = strchr(p, '\n');
        size_t len = nl ? (size_t)(nl - p) : strlen(p);
        if (len && p[len - 1] == '\r') {
            len--;
        }
        bool drop = len > 0 && p[0] != '#' && linea_de_ficha(p, len, tipo, id);
        quito = quito || drop;
        if (!drop && w + len + 1 < TAM_RESUMEN) {
            memcpy(out + w, p, len);
            w += len;
            out[w++] = '\n';
        }
        if (!nl) {
            break;
        }
        p = nl + 1;
    }
    bool ok = false;
    if (quito) {
        FILE *f = fopen(RUTA_RESUMEN, "wb");
        if (f) {
            fwrite(out, 1, w, f);
            fclose(f);
            ok = true;
        }
    }
    free(out);
    free(buf);
    return ok;
}
