#include "ui.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "almacen.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "pantalla.h"
#include "placa.h"

static const char *TAG = "ui";

#define MAX_ITEMS 40
#define ESCALA 2

typedef struct {
    char tipo[12];
    char titulo[80];
    char fecha[16];
    char id[40];
} item_t;

static item_t s_items[MAX_ITEMS];
static int s_n = 0;
static int s_sel = 0;
static ui_pantalla_t s_pantalla = UI_HOME;
static uint8_t *s_fb = NULL;

// 5x7, columnas, bit 0 = arriba. Hueco ' ' (32) a 'Z' (90); el resto se salta.
#define GLIFO_0 32

static const uint8_t FONT[][5] = {
    {0, 0, 0, 0, 0},
    {0x00, 0x00, 0x5F, 0x00, 0x00},
    {0x00, 0x07, 0x00, 0x07, 0x00},
    {0x14, 0x7F, 0x14, 0x7F, 0x14},
    {0x24, 0x2A, 0x7F, 0x2A, 0x12},
    {0x23, 0x13, 0x08, 0x64, 0x62},
    {0x36, 0x49, 0x55, 0x22, 0x50},
    {0x00, 0x05, 0x03, 0x00, 0x00},
    {0x00, 0x1C, 0x22, 0x41, 0x00},
    {0x00, 0x41, 0x22, 0x1C, 0x00},
    {0x14, 0x08, 0x3E, 0x08, 0x14},
    {0x08, 0x08, 0x3E, 0x08, 0x08},
    {0x00, 0x50, 0x30, 0x00, 0x00},
    {0x08, 0x08, 0x08, 0x08, 0x08},
    {0x00, 0x60, 0x60, 0x00, 0x00},
    {0x20, 0x10, 0x08, 0x04, 0x02},
    {0x3E, 0x51, 0x49, 0x45, 0x3E},
    {0x00, 0x42, 0x7F, 0x40, 0x00},
    {0x42, 0x61, 0x51, 0x49, 0x46},
    {0x21, 0x41, 0x45, 0x4B, 0x31},
    {0x18, 0x14, 0x12, 0x7F, 0x10},
    {0x27, 0x45, 0x45, 0x45, 0x39},
    {0x3C, 0x4A, 0x49, 0x49, 0x30},
    {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36},
    {0x06, 0x49, 0x49, 0x29, 0x1E},
    {0x00, 0x36, 0x36, 0x00, 0x00},
    {0x00, 0x56, 0x36, 0x00, 0x00},
    {0x08, 0x14, 0x22, 0x41, 0x00},
    {0x14, 0x14, 0x14, 0x14, 0x14},
    {0x00, 0x41, 0x22, 0x14, 0x08},
    {0x02, 0x01, 0x51, 0x09, 0x06},
    {0x32, 0x49, 0x79, 0x41, 0x3E},
    {0x7E, 0x11, 0x11, 0x11, 0x7E},
    {0x7F, 0x49, 0x49, 0x49, 0x36},
    {0x3E, 0x41, 0x41, 0x41, 0x22},
    {0x7F, 0x41, 0x41, 0x22, 0x1C},
    {0x7F, 0x49, 0x49, 0x49, 0x41},
    {0x7F, 0x09, 0x09, 0x09, 0x01},
    {0x3E, 0x41, 0x49, 0x49, 0x7A},
    {0x7F, 0x08, 0x08, 0x08, 0x7F},
    {0x00, 0x41, 0x7F, 0x41, 0x00},
    {0x20, 0x40, 0x41, 0x3F, 0x01},
    {0x7F, 0x08, 0x14, 0x22, 0x41},
    {0x7F, 0x40, 0x40, 0x40, 0x40},
    {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    {0x7F, 0x04, 0x08, 0x10, 0x7F},
    {0x3E, 0x41, 0x41, 0x41, 0x3E},
    {0x7F, 0x09, 0x09, 0x09, 0x06},
    {0x3E, 0x41, 0x51, 0x21, 0x5E},
    {0x7F, 0x09, 0x19, 0x29, 0x46},
    {0x46, 0x49, 0x49, 0x49, 0x31},
    {0x01, 0x01, 0x7F, 0x01, 0x01},
    {0x3F, 0x40, 0x40, 0x40, 0x3F},
    {0x1F, 0x20, 0x40, 0x20, 0x1F},
    {0x3F, 0x40, 0x38, 0x40, 0x3F},
    {0x63, 0x14, 0x08, 0x14, 0x63},
    {0x07, 0x08, 0x70, 0x08, 0x07},
    {0x61, 0x51, 0x49, 0x45, 0x43},
};

_Static_assert(sizeof(FONT) / sizeof(FONT[0]) == (size_t)('Z' - GLIFO_0 + 1), "fuente 32..Z");

static void pixel(int x, int y, bool negro)
{
    if (x < 0 || y < 0 || x >= EPD_ANCHO || y >= EPD_ALTO || !negro) {
        return;
    }
    size_t i = (size_t)y * (EPD_ANCHO / 8) + (size_t)(x / 8);
    s_fb[i] &= (uint8_t)~(0x80 >> (x % 8));
}

static void texto(int x, int y, const char *s)
{
    for (; s && *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (c >= 'a' && c <= 'z') {
            c = (unsigned char)(c - 'a' + 'A');
        }
        const uint8_t *g = NULL;
        if (c >= GLIFO_0 && c <= 'Z') {
            g = FONT[c - GLIFO_0];
        }
        for (int col = 0; col < 5; col++) {
            uint8_t bits = g ? g[col] : 0;
            for (int row = 0; row < 7; row++) {
                if (bits & (1 << row)) {
                    for (int sy = 0; sy < ESCALA; sy++) {
                        for (int sx = 0; sx < ESCALA; sx++) {
                            pixel(x + col * ESCALA + sx, y + row * ESCALA + sy, true);
                        }
                    }
                }
            }
        }
        x += 6 * ESCALA;
        if (x > EPD_ANCHO - 6 * ESCALA) {
            break;
        }
    }
}

static void limpiar(void)
{
    memset(s_fb, 0xFF, EPD_BYTES);
}

static void linea_item(int y, int i, bool marca)
{
    if (i < 0 || i >= s_n) {
        return;
    }
    char buf[120];
    snprintf(buf, sizeof(buf), "%s%s  %s  %s", marca ? ">" : " ", s_items[i].tipo, s_items[i].fecha,
             s_items[i].titulo);
    texto(8, y, buf);
}

void ui_cargar_cache(void)
{
    s_n = 0;
    FILE *f = fopen(RUTA_RESUMEN, "r");
    if (!f) {
        ESP_LOGI(TAG, "Sin cache de resumen");
        return;
    }
    char linea[200];
    while (s_n < MAX_ITEMS && fgets(linea, sizeof(linea), f)) {
        if (linea[0] == '#' || linea[0] == '\n') {
            continue;
        }
        char *tipo = linea;
        char *titulo = strchr(tipo, '\t');
        if (!titulo) {
            continue;
        }
        *titulo++ = '\0';
        char *fecha = strchr(titulo, '\t');
        if (!fecha) {
            continue;
        }
        *fecha++ = '\0';
        char *id = strchr(fecha, '\t');
        if (!id) {
            continue;
        }
        *id++ = '\0';
        char *nl = strchr(id, '\n');
        if (nl) {
            *nl = '\0';
        }
        item_t *it = &s_items[s_n++];
        snprintf(it->tipo, sizeof(it->tipo), "%s", tipo);
        snprintf(it->titulo, sizeof(it->titulo), "%s", titulo);
        snprintf(it->fecha, sizeof(it->fecha), "%s", fecha);
        snprintf(it->id, sizeof(it->id), "%s", id);
    }
    fclose(f);
    if (s_sel >= s_n) {
        s_sel = s_n ? s_n - 1 : 0;
    }
    ESP_LOGI(TAG, "%d elementos en cache", s_n);
}

void ui_pintar(ui_pantalla_t pantalla)
{
    if (!s_fb) {
        s_fb = heap_caps_malloc(EPD_BYTES, MALLOC_CAP_SPIRAM);
        if (!s_fb) {
            return;
        }
    }
    s_pantalla = pantalla;
    limpiar();
    if (pantalla == UI_LOCK) {
        texto(8, 8, "MaxSecIA");
        texto(8, 32, "Pendientes");
        int y = 64;
        int n = 0;
        for (int i = 0; i < s_n && n < 10; i++) {
            if (strcmp(s_items[i].tipo, "nota") == 0 || strcmp(s_items[i].tipo, "idea") == 0) {
                continue;
            }
            linea_item(y, i, false);
            y += 18;
            n++;
        }
        if (n == 0) {
            texto(8, 64, "Nada pendiente");
        }
    } else if (pantalla == UI_DETALLE && s_n > 0) {
        texto(8, 8, s_items[s_sel].tipo);
        texto(8, 32, s_items[s_sel].fecha);
        texto(8, 64, s_items[s_sel].titulo);
        texto(8, 440, "BOOT atras");
    } else if (pantalla == UI_LISTA) {
        texto(8, 8, "Lista");
        int base = s_sel - 4;
        if (base < 0) {
            base = 0;
        }
        for (int row = 0; row < 12 && base + row < s_n; row++) {
            linea_item(36 + row * 28, base + row, base + row == s_sel);
        }
        if (s_n == 0) {
            texto(8, 48, "Cache vacia");
        }
    } else {
        texto(8, 8, "MaxSecIA");
        texto(8, 36, "Select lista   Record grabar   PWR bloqueo");
        int y = 72;
        int shown = s_n < 8 ? s_n : 8;
        for (int i = 0; i < shown; i++) {
            linea_item(y, i, false);
            y += 28;
        }
        if (s_n == 0) {
            texto(8, 80, "Sin datos. Conecta y sincroniza.");
        }
    }
    pantalla_mostrar(s_fb);
}

void ui_mover(int delta)
{
    if (s_pantalla != UI_LISTA || s_n == 0) {
        return;
    }
    s_sel += delta;
    if (s_sel < 0) {
        s_sel = 0;
    }
    if (s_sel >= s_n) {
        s_sel = s_n - 1;
    }
    ui_pintar(UI_LISTA);
}

void ui_abrir(void)
{
    if (s_pantalla == UI_HOME) {
        ui_pintar(UI_LISTA);
    } else if (s_pantalla == UI_LISTA && s_n > 0) {
        ui_pintar(UI_DETALLE);
    }
}

void ui_atras(void)
{
    if (s_pantalla == UI_DETALLE) {
        ui_pintar(UI_LISTA);
    } else if (s_pantalla == UI_LISTA || s_pantalla == UI_LOCK) {
        ui_pintar(UI_HOME);
    }
}

ui_pantalla_t ui_pantalla(void)
{
    return s_pantalla;
}
