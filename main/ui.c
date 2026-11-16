// UI en portrait 480x800 (estilo FolloUp/Chroninkle: menú vertical, iconos, lock).
// Se rota a 800x480 al volcar. No usa código GPLv3 de Chroninkle.
#include "ui.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "almacen.h"
#include "energia.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "pantalla.h"
#include "placa.h"
#include "red.h"

static const char *TAG = "ui";

#define MAX_ITEMS 40
#define UI_W 480
#define UI_H 800
#define UI_BYTES ((UI_W * UI_H) / 8)

#define MARGEN 24
#define MENU_N 5

typedef struct {
    char tipo[12];
    char titulo[80];
    char fecha[16];
    char id[40];
} item_t;

typedef enum {
    FILTRO_FOLLOW = 0,
    FILTRO_TODOS,
    FILTRO_NOTAS,
    FILTRO_TOPICS,
    FILTRO_BOOKS,
} filtro_t;

static item_t s_items[MAX_ITEMS];
static int s_n = 0;
static int s_sel = 0;
static int s_base = 0;
static int s_menu = 0;
static filtro_t s_filtro = FILTRO_TODOS;
static int s_vista[MAX_ITEMS];
static int s_nvista = 0;
static ui_pantalla_t s_pantalla = UI_HOME;
static uint8_t *s_logic = NULL;
static uint8_t *s_fb = NULL;

#define MAX_LIBROS 16
static item_t s_libros[MAX_LIBROS];
static int s_nlibros = 0;
static char s_libro_texto[480];

// 5x7, columnas, bit0 = arriba. ' ' .. 'Z'
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
    {0x7F, 0x41, 0x41, 0x41, 0x00},
    {0x02, 0x04, 0x08, 0x10, 0x20},
    {0x00, 0x41, 0x41, 0x41, 0x7F},
    {0x04, 0x02, 0x01, 0x02, 0x04},
    {0x40, 0x40, 0x40, 0x40, 0x40},
    {0x00, 0x01, 0x02, 0x04, 0x00},
    {0x20, 0x54, 0x54, 0x54, 0x78},
    {0x7F, 0x48, 0x44, 0x44, 0x38},
    {0x38, 0x44, 0x44, 0x44, 0x20},
    {0x38, 0x44, 0x44, 0x48, 0x7F},
    {0x38, 0x54, 0x54, 0x54, 0x18},
    {0x08, 0x7E, 0x09, 0x01, 0x02},
    {0x0C, 0x52, 0x52, 0x52, 0x3E},
    {0x7F, 0x08, 0x04, 0x04, 0x78},
    {0x00, 0x44, 0x7D, 0x40, 0x00},
    {0x20, 0x40, 0x44, 0x3D, 0x00},
    {0x7F, 0x10, 0x28, 0x44, 0x00},
    {0x00, 0x41, 0x7F, 0x40, 0x00},
    {0x7C, 0x04, 0x18, 0x04, 0x78},
    {0x7C, 0x08, 0x04, 0x04, 0x78},
    {0x38, 0x44, 0x44, 0x44, 0x38},
    {0x7C, 0x14, 0x14, 0x14, 0x08},
    {0x08, 0x14, 0x14, 0x18, 0x7C},
    {0x7C, 0x08, 0x04, 0x04, 0x08},
    {0x48, 0x54, 0x54, 0x54, 0x20},
    {0x04, 0x3F, 0x44, 0x40, 0x20},
    {0x3C, 0x40, 0x40, 0x20, 0x7C},
    {0x1C, 0x20, 0x40, 0x20, 0x1C},
    {0x3C, 0x40, 0x30, 0x40, 0x3C},
    {0x44, 0x28, 0x10, 0x28, 0x44},
    {0x0C, 0x50, 0x50, 0x50, 0x3C},
    {0x44, 0x64, 0x54, 0x4C, 0x44},
};
_Static_assert(sizeof(FONT) / sizeof(FONT[0]) == (size_t)('z' - GLIFO_0 + 1), "fuente");

static bool s_tinta_blanca = false;
static bool s_clip = false;
static int s_cx0, s_cy0, s_cx1, s_cy1;

static void px(int x, int y, bool negro)
{
    if (s_tinta_blanca) {
        negro = !negro;
    }
    if (x < 0 || y < 0 || x >= UI_W || y >= UI_H) {
        return;
    }
    if (s_clip && (x < s_cx0 || y < s_cy0 || x >= s_cx1 || y >= s_cy1)) {
        return;
    }
    size_t i = (size_t)y * (UI_W / 8) + (size_t)(x / 8);
    uint8_t m = (uint8_t)(0x80 >> (x % 8));
    if (negro) {
        s_logic[i] &= (uint8_t)~m;
    } else {
        s_logic[i] |= m;
    }
}

static void hline(int x, int y, int w, bool negro)
{
    for (int i = 0; i < w; i++) {
        px(x + i, y, negro);
    }
}

static void vline(int x, int y, int h, bool negro)
{
    for (int i = 0; i < h; i++) {
        px(x, y + i, negro);
    }
}

static void rect(int x, int y, int w, int h, bool fill, bool negro)
{
    if (fill) {
        for (int yy = 0; yy < h; yy++) {
            hline(x, y + yy, w, negro);
        }
    } else {
        hline(x, y, w, negro);
        hline(x, y + h - 1, w, negro);
        vline(x, y, h, negro);
        vline(x + w - 1, y, h, negro);
    }
}

static unsigned char tomar_caracter(const char **ps)
{
    const unsigned char *p = (const unsigned char *)*ps;
    if (!p || !*p) {
        return 0;
    }
    if (*p < 0x80) {
        *ps += 1;
        return *p;
    }
    if (*p == 0xC3 && p[1]) {
        unsigned char n = p[1];
        *ps += 2;
        if (n == 0x91) {
            return 'N';
        }
        if (n == 0xB1) {
            return 'n';
        }
        if (n >= 0x80 && n <= 0x85) {
            return 'A';
        }
        if (n >= 0xA0 && n <= 0xA5) {
            return 'a';
        }
        if (n >= 0x88 && n <= 0x8B) {
            return 'E';
        }
        if (n >= 0xA8 && n <= 0xAB) {
            return 'e';
        }
        if (n >= 0x8C && n <= 0x8F) {
            return 'I';
        }
        if (n >= 0xAC && n <= 0xAF) {
            return 'i';
        }
        if ((n >= 0x92 && n <= 0x96) || n == 0x98) {
            return 'O';
        }
        if ((n >= 0xB2 && n <= 0xB6) || n == 0xB8) {
            return 'o';
        }
        if (n >= 0x99 && n <= 0x9C) {
            return 'U';
        }
        if (n >= 0xB9 && n <= 0xBC) {
            return 'u';
        }
        return '?';
    }
    if ((*p & 0xE0) == 0xC0 && p[1]) {
        *ps += 2;
    } else if ((*p & 0xF0) == 0xE0 && p[1] && p[2]) {
        *ps += 3;
    } else {
        *ps += 1;
    }
    return '?';
}

static void texto_esc(int x, int y, const char *s, int esc)
{
    while (s && *s) {
        unsigned char c = tomar_caracter(&s);
        if (!c) {
            break;
        }
        const uint8_t *g = NULL;
        if (c >= GLIFO_0 && c <= 'z') {
            g = FONT[c - GLIFO_0];
        }
        for (int col = 0; col < 5; col++) {
            uint8_t bits = g ? g[col] : 0;
            for (int row = 0; row < 7; row++) {
                if (bits & (1u << row)) {
                    for (int sy = 0; sy < esc; sy++) {
                        for (int sx = 0; sx < esc; sx++) {
                            px(x + col * esc + sx, y + row * esc + sy, true);
                        }
                    }
                }
            }
        }
        x += 6 * esc;
        if (x > UI_W - 6 * esc) {
            break;
        }
    }
}

static void texto(int x, int y, const char *s)
{
    texto_esc(x, y, s, 2);
}

static void titulo(int x, int y, const char *s)
{
    texto_esc(x, y, s, 2);
}

// Entra en la caja. Corta por palabras y no dibuja fuera.
static void texto_caja(int x, int y, int w, int h, int esc, const char *s)
{
    if (!s || w < 6 * esc || h < 7 * esc) {
        return;
    }
    bool prev = s_clip;
    int px0 = s_cx0, py0 = s_cy0, px1 = s_cx1, py1 = s_cy1;
    s_clip = true;
    s_cx0 = x;
    s_cy0 = y;
    s_cx1 = x + w;
    s_cy1 = y + h;
    const char *p = s;
    int yy = y;
    while (*p && yy + 7 * esc <= y + h) {
        int n = 0;
        int usado = 0;
        int corte = 0;
        while (p[n] && p[n] != '\n' && usado + 6 * esc <= w && n < 80) {
            if (p[n] == ' ') {
                corte = n;
            }
            usado += 6 * esc;
            n++;
        }
        if (p[n] && p[n] != '\n' && corte > 0) {
            n = corte;
        }
        char linea[81];
        if (n >= (int)sizeof(linea)) {
            n = (int)sizeof(linea) - 1;
        }
        memcpy(linea, p, (size_t)n);
        linea[n] = '\0';
        texto_esc(x, yy, linea, esc);
        p += n;
        if (*p == '\n' || *p == ' ') {
            p++;
        }
        yy += 8 * esc;
    }
    s_clip = prev;
    s_cx0 = px0;
    s_cy0 = py0;
    s_cx1 = px1;
    s_cy1 = py1;
}

static void icono_check(int x, int y)
{
    rect(x, y, 28, 28, false, true);
    // check mark
    for (int i = 0; i < 8; i++) {
        px(x + 6 + i, y + 14 + i / 2, true);
    }
    for (int i = 0; i < 14; i++) {
        px(x + 14 + i, y + 18 - i, true);
    }
}

static void icono_sticky(int x, int y)
{
    rect(x + 2, y + 2, 26, 26, false, true);
    rect(x, y, 26, 26, false, true);
    hline(x + 4, y + 8, 18, true);
    hline(x + 4, y + 14, 14, true);
    hline(x + 4, y + 20, 10, true);
}

static void icono_nota(int x, int y)
{
    rect(x + 4, y, 20, 28, false, true);
    hline(x + 8, y + 8, 12, true);
    hline(x + 8, y + 14, 12, true);
    hline(x + 8, y + 20, 8, true);
}

static void icono_cal(int x, int y)
{
    rect(x, y + 4, 28, 24, false, true);
    hline(x, y + 12, 28, true);
    vline(x + 8, y, 6, true);
    vline(x + 20, y, 6, true);
    px(x + 8, y + 18, true);
    px(x + 14, y + 18, true);
    px(x + 20, y + 18, true);
}

static void icono_idea(int x, int y)
{
    // bombilla simple
    for (int dy = 0; dy < 12; dy++) {
        int w = 8 + (dy < 6 ? dy : 11 - dy);
        hline(x + 14 - w / 2, y + 4 + dy, w, true);
    }
    rect(x + 10, y + 16, 8, 8, false, true);
    hline(x + 10, y + 20, 8, true);
}

static void icono_lista(int x, int y)
{
    for (int i = 0; i < 3; i++) {
        rect(x, y + i * 10, 6, 6, true, true);
        hline(x + 10, y + 2 + i * 10, 16, true);
    }
}

static void icono_tipo(const char *tipo, int x, int y)
{
    if (strcmp(tipo, "deck") == 0 || strcmp(tipo, "pendiente") == 0) {
        icono_check(x, y);
    } else if (strcmp(tipo, "evento") == 0) {
        icono_cal(x, y);
    } else if (strcmp(tipo, "idea") == 0) {
        icono_idea(x, y);
    } else if (strcmp(tipo, "nota") == 0) {
        icono_nota(x, y);
    } else if (strcmp(tipo, "libro") == 0 || strcmp(tipo, "articulo") == 0) {
        icono_lista(x, y);
    } else {
        icono_sticky(x, y);
    }
}

static bool pasa_filtro(const item_t *it, filtro_t f)
{
    switch (f) {
    case FILTRO_FOLLOW:
        return strcmp(it->tipo, "deck") == 0 || strcmp(it->tipo, "evento") == 0;
    case FILTRO_TODOS:  // Tareas
        return strcmp(it->tipo, "deck") == 0 || strcmp(it->tipo, "pendiente") == 0;
    case FILTRO_NOTAS:
        return strcmp(it->tipo, "nota") == 0 || strcmp(it->tipo, "idea") == 0;
    case FILTRO_TOPICS:
    case FILTRO_BOOKS:
        return false;
    default:
        return false;
    }
}

static int contar_filtro(filtro_t f)
{
    int n = 0;
    for (int i = 0; i < s_n; i++) {
        if (pasa_filtro(&s_items[i], f)) {
            n++;
        }
    }
    return n;
}

static void cargar_libros(void)
{
    s_nlibros = 0;
    DIR *d = opendir("/sdcard/libros");
    if (!d) {
        return;
    }
    struct dirent *e;
    while (s_nlibros < MAX_LIBROS && (e = readdir(d)) != NULL) {
        size_t n = strlen(e->d_name);
        bool txt = n >= 5 && strcasecmp(e->d_name + n - 4, ".txt") == 0;
        bool md = n >= 4 && strcasecmp(e->d_name + n - 3, ".md") == 0;
        if (!txt && !md) {
            continue;
        }
        item_t *it = &s_libros[s_nlibros++];
        snprintf(it->tipo, sizeof(it->tipo), md ? "articulo" : "libro");
        snprintf(it->titulo, sizeof(it->titulo), "%s", e->d_name);
        snprintf(it->fecha, sizeof(it->fecha), "%s", md ? "md" : "txt");
        snprintf(it->id, sizeof(it->id), "%s", e->d_name);
    }
    closedir(d);
}

static item_t *item_en(int indice)
{
    if (s_filtro == FILTRO_BOOKS) {
        return &s_libros[indice];
    }
    return &s_items[indice];
}

static void leer_libro(const item_t *it)
{
    s_libro_texto[0] = '\0';
    char ruta[96];
    snprintf(ruta, sizeof(ruta), "/sdcard/libros/%s", it->id);
    FILE *f = fopen(ruta, "r");
    if (!f) {
        snprintf(s_libro_texto, sizeof(s_libro_texto), "%s", it->titulo);
        return;
    }
    size_t n = fread(s_libro_texto, 1, sizeof(s_libro_texto) - 1, f);
    s_libro_texto[n] = '\0';
    fclose(f);
    for (size_t i = 0; i < n; i++) {
        if (s_libro_texto[i] == '\r') {
            s_libro_texto[i] = ' ';
        }
    }
}

static void reconstruir_vista(void)
{
    s_nvista = 0;
    if (s_filtro == FILTRO_BOOKS) {
        cargar_libros();
        for (int i = 0; i < s_nlibros && s_nvista < MAX_ITEMS; i++) {
            s_vista[s_nvista++] = i;
        }
    } else {
        for (int i = 0; i < s_n; i++) {
            if (pasa_filtro(&s_items[i], s_filtro)) {
                s_vista[s_nvista++] = i;
            }
        }
    }
    if (s_sel >= s_nvista) {
        s_sel = s_nvista ? s_nvista - 1 : 0;
    }
}

static void limpiar(void)
{
    memset(s_logic, 0xFF, UI_BYTES);
}

// Portrait (lx,ly) -> landscape físico 800x480 (90° CW).
static void volcar_rotado(void)
{
    memset(s_fb, 0xFF, EPD_BYTES);
    for (int ly = 0; ly < UI_H; ly++) {
        for (int lx = 0; lx < UI_W; lx++) {
            size_t si = (size_t)ly * (UI_W / 8) + (size_t)(lx / 8);
            bool negro = (s_logic[si] & (0x80 >> (lx % 8))) == 0;
            if (!negro) {
                continue;
            }
            int px = ly;
            int py = UI_W - 1 - lx;
            if (px < 0 || py < 0 || px >= EPD_ANCHO || py >= EPD_ALTO) {
                continue;
            }
            size_t di = (size_t)py * (EPD_ANCHO / 8) + (size_t)(px / 8);
            s_fb[di] &= (uint8_t)~(0x80 >> (px % 8));
        }
    }
    pantalla_actualizar(s_fb);
}

static void barra_progreso(int x, int y, int w, int h, int pct)
{
    if (pct < 0) {
        pct = 0;
    }
    if (pct > 100) {
        pct = 100;
    }
    rect(x, y, w, h, false, true);
    int fw = (w - 4) * pct / 100;
    if (fw > 0) {
        rect(x + 2, y + 2, fw, h - 4, true, true);
    }
}

static void icono_wifi(int x, int y, bool on)
{
    if (!on) {
        for (int i = 0; i < 12; i++) {
            px(x + 2 + i, y + 2 + i, true);
            px(x + 14 - i, y + 2 + i, true);
        }
        return;
    }
    px(x + 8, y + 14, true);
    px(x + 7, y + 14, true);
    px(x + 8, y + 13, true);
    for (int i = -3; i <= 3; i++) {
        px(x + 8 + i, y + 9, true);
    }
    for (int i = -6; i <= 6; i++) {
        px(x + 8 + i, y + 4, true);
    }
}

static void icono_bateria(int x, int y, int pct)
{
    rect(x, y, 24, 12, false, true);
    rect(x + 24, y + 3, 3, 6, true, true);
    if (pct > 0) {
        int w = 20 * pct / 100;
        if (w < 1) {
            w = 1;
        }
        rect(x + 2, y + 2, w, 8, true, true);
    }
}

static void barra_estado(void)
{
    // Gris ~50 % (trama), no negro sólido.
    for (int y = 0; y < 44; y++) {
        for (int x = 0; x < UI_W; x++) {
            if (((x + y) & 1) == 0) {
                px(x, y, true);
            }
        }
    }
    hline(0, 44, UI_W, true);
    char hora[8] = "--:--";
    time_t now = time(NULL);
    struct tm tm;
    if (now > 100000 && localtime_r(&now, &tm)) {
        snprintf(hora, sizeof(hora), "%02d:%02d", tm.tm_hour, tm.tm_min);
    }
    texto_esc(14, 14, hora, 2);
    icono_wifi(UI_W - 92, 14, red_conectada());
    int mv = energia_bateria_mv();
    int pct = -1;
    if (mv >= 3300 && mv <= 4350) {
        pct = (mv - 3300) * 100 / 900;
        if (pct > 100) {
            pct = 100;
        }
        if (pct < 0) {
            pct = 0;
        }
    }
    icono_bateria(UI_W - 48, 16, pct);
}

static const char *NOMBRES_MENU[MENU_N] = {"Follow", "Tareas", "Notas", "Topics", "Books"};
static const char *TITULOS_MENU[MENU_N] = {"Follow up", "Tareas", "Notas", "Topics", "Books"};
static const filtro_t FILTROS_MENU[MENU_N] = {
    FILTRO_FOLLOW, FILTRO_TODOS, FILTRO_NOTAS, FILTRO_TOPICS, FILTRO_BOOKS,
};

static void sincronizar_filtro_menu(void)
{
    s_filtro = FILTROS_MENU[s_menu];
    s_sel = 0;
    s_base = 0;
    reconstruir_vista();
}

static void icono_menu(int i, int x, int y)
{
    switch (i) {
    case 0:
        icono_sticky(x, y);
        break;
    case 1:
        icono_check(x, y);
        break;
    case 2:
        icono_nota(x, y);
        break;
    case 3:
        icono_idea(x, y);
        break;
    default:
        icono_lista(x, y);
        break;
    }
}

#define BARRA_Y 712
#define BARRA_H 88

static int menu_activo(void)
{
    if (s_pantalla == UI_HOME) {
        return s_menu;
    }
    for (int i = 0; i < MENU_N; i++) {
        if (FILTROS_MENU[i] == s_filtro) {
            return i;
        }
    }
    return s_menu;
}

static void barra_menu(void)
{
    hline(0, BARRA_Y, UI_W, true);
    int celda = UI_W / MENU_N;
    int activo = menu_activo();
    for (int i = 0; i < MENU_N; i++) {
        int x = i * celda;
        bool sel = (i == activo);
        if (sel) {
            rect(x + 2, BARRA_Y + 4, celda - 4, BARRA_H - 8, true, true);
            s_tinta_blanca = true;
        }
        icono_menu(i, x + (celda - 28) / 2, BARRA_Y + 8);
        int tw = (int)strlen(NOMBRES_MENU[i]) * 12;
        texto_esc(x + (celda - tw) / 2, BARRA_Y + 46, NOMBRES_MENU[i], 2);
        s_tinta_blanca = false;
    }
}

static void mensaje_vacio(void)
{
    const char *msg = "Nada en esta lista";
    if (s_filtro == FILTRO_TOPICS) {
        msg = "Los temas llegan cuando el servidor etiqueta lo grabado. Todavia no hay lista.";
    } else if (s_filtro == FILTRO_BOOKS) {
        msg = "No hay libros ni articulos. Deja .txt o .md en /sdcard/libros.";
    }
    texto_caja(MARGEN, 160, UI_W - 2 * MARGEN, 160, 2, msg);
}

// con_selector: en lista se marca el ítem; en home solo se previsualiza la sección.
static void pintar_cuerpo_lista(bool con_selector)
{
    titulo(MARGEN, 56, TITULOS_MENU[menu_activo()]);
    char badge[24];
    snprintf(badge, sizeof(badge), "%d", s_nvista);
    texto_esc(UI_W - MARGEN - 48, 60, badge, 2);

    if (s_nvista == 0) {
        mensaje_vacio();
        return;
    }

    const int fila = 72;
    const int y0 = 96;
    const int filas = (BARRA_Y - 40 - y0) / fila;
    if (con_selector) {
        if (s_sel < s_base) {
            s_base = s_sel;
        }
        if (s_sel >= s_base + filas) {
            s_base = s_sel - filas + 1;
        }
    } else {
        s_base = 0;
    }
    if (s_base < 0) {
        s_base = 0;
    }
    int y = y0;
    int ancho = UI_W - 2 * MARGEN;
    for (int row = 0; row < filas && s_base + row < s_nvista; row++) {
        int vi = s_base + row;
        int ii = s_vista[vi];
        bool sel = con_selector && (vi == s_sel);
        item_t *it = item_en(ii);
        rect(MARGEN - 4, y, ancho + 8, fila - 6, false, true);
        if (sel) {
            rect(MARGEN - 4, y, 8, fila - 6, true, true);
        }
        icono_tipo(it->tipo, MARGEN + 12, y + 16);
        texto_caja(MARGEN + 52, y + 8, ancho - 64, 32, 2, it->titulo);
        const char *sub = it->fecha[0] ? it->fecha : it->tipo;
        texto_caja(MARGEN + 52, y + 42, ancho - 64, 18, 2, sub);
        y += fila;
    }
}

static void pintar_home(void)
{
    sincronizar_filtro_menu();
    limpiar();
    barra_estado();
    pintar_cuerpo_lista(false);
    texto_esc(MARGEN, BARRA_Y - 28, "Rocker cambia seccion  ·  Clic entra", 2);
    barra_menu();
    volcar_rotado();
}

static void pintar_lista(void)
{
    limpiar();
    barra_estado();
    pintar_cuerpo_lista(true);
    texto_esc(MARGEN, BARRA_Y - 28, "Rocker mueve  ·  Clic abre  ·  Largo sale", 2);
    barra_menu();
    volcar_rotado();
}

static void pintar_detalle(void)
{
    limpiar();
    barra_estado();
    if (s_nvista == 0) {
        texto(MARGEN, 64, "Sin elemento");
        barra_menu();
        volcar_rotado();
        return;
    }
    int ii = s_vista[s_sel];
    item_t *it = item_en(ii);
    if (s_filtro == FILTRO_BOOKS) {
        leer_libro(it);
    }
    int alto = BARRA_Y - 64;
    rect(MARGEN, 52, UI_W - 2 * MARGEN, alto, false, true);
    rect(MARGEN + 4, 56, UI_W - 2 * MARGEN - 8, alto - 8, false, true);
    icono_tipo(it->tipo, MARGEN + 20, 72);
    texto_esc(MARGEN + 60, 78, it->tipo, 2);
    if (it->fecha[0]) {
        texto_caja(MARGEN + 20, 112, UI_W - 2 * MARGEN - 40, 20, 2, it->fecha);
    }
    hline(MARGEN + 20, 140, UI_W - 2 * MARGEN - 40, true);
    const char *cuerpo = (s_filtro == FILTRO_BOOKS && s_libro_texto[0]) ? s_libro_texto : it->titulo;
    texto_caja(MARGEN + 20, 156, UI_W - 2 * MARGEN - 40, alto - 124, 2, cuerpo);
    barra_menu();
    volcar_rotado();
}

static void pintar_lock(void)
{
    limpiar();
    barra_estado();
    time_t now = time(NULL);
    struct tm tm;
    char line[48];
    if (now > 100000 && localtime_r(&now, &tm)) {
        static const char *DIAS[] = {"Domingo", "Lunes", "Martes", "Miercoles", "Jueves", "Viernes", "Sabado"};
        snprintf(line, sizeof(line), "%s %02d/%02d", DIAS[tm.tm_wday], tm.tm_mday, tm.tm_mon + 1);
        texto(MARGEN, 56, line);
    }

    int n = contar_filtro(FILTRO_TODOS);
    if (n == 0) {
        texto(MARGEN, 110, "Todo al dia");
    } else {
        snprintf(line, sizeof(line), "%d pendientes", n);
        texto(MARGEN, 100, line);
        int y = 140;
        int shown = 0;
        int ancho = UI_W - 2 * MARGEN;
        for (int i = 0; i < s_n && shown < 7; i++) {
            if (!pasa_filtro(&s_items[i], FILTRO_TODOS)) {
                continue;
            }
            rect(MARGEN, y, ancho, 72, false, true);
            icono_check(MARGEN + 10, y + 20);
            texto_caja(MARGEN + 48, y + 10, ancho - 60, 48, 2, s_items[i].titulo);
            y += 80;
            shown++;
        }
        if (n > shown) {
            snprintf(line, sizeof(line), "+%d mas", n - shown);
            texto_caja(MARGEN, y + 4, ancho, 20, 2, line);
        }
    }
    texto_caja(MARGEN, UI_H - 40, UI_W - 2 * MARGEN, 20, 2, "Ruleta desbloquea  ·  BOOT graba");
    volcar_rotado();
}

int ui_num_items(void)
{
    return s_n;
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
    reconstruir_vista();
    ESP_LOGI(TAG, "%d elementos en cache", s_n);
}

static bool asegurar_bufs(void)
{
    if (!s_logic) {
        s_logic = heap_caps_malloc(UI_BYTES, MALLOC_CAP_SPIRAM);
    }
    if (!s_fb) {
        s_fb = heap_caps_malloc(EPD_BYTES, MALLOC_CAP_SPIRAM);
    }
    return s_logic && s_fb;
}

void ui_pintar(ui_pantalla_t pantalla)
{
    if (!asegurar_bufs()) {
        return;
    }
    s_pantalla = pantalla;
    if (pantalla == UI_LOCK) {
        pintar_lock();
    } else if (pantalla == UI_DETALLE) {
        pintar_detalle();
    } else if (pantalla == UI_LISTA) {
        pintar_lista();
    } else {
        pintar_home();
    }
}

void ui_repintar(void)
{
    ui_pintar(s_pantalla);
}

void ui_mover(int delta)
{
    if (s_pantalla == UI_HOME) {
        int antes = s_menu;
        s_menu += delta;
        if (s_menu < 0) {
            s_menu = 0;
        }
        if (s_menu >= MENU_N) {
            s_menu = MENU_N - 1;
        }
        if (s_menu != antes) {
            sincronizar_filtro_menu();
            ui_pintar(UI_HOME);
        }
        return;
    }
    // Dentro de una sección solo se mueve el ítem; el menú inferior no cambia.
    if (s_pantalla == UI_LISTA && s_nvista > 0) {
        int antes = s_sel;
        s_sel += delta;
        if (s_sel < 0) {
            s_sel = 0;
        }
        if (s_sel >= s_nvista) {
            s_sel = s_nvista - 1;
        }
        if (s_sel != antes) {
            ui_pintar(UI_LISTA);
        }
    }
}

void ui_abrir(void)
{
    if (s_pantalla == UI_HOME) {
        sincronizar_filtro_menu();
        ui_pintar(UI_LISTA);
    } else if (s_pantalla == UI_LISTA && s_nvista > 0) {
        ui_pintar(UI_DETALLE);
    }
}

void ui_atras(void)
{
    if (s_pantalla == UI_DETALLE) {
        ui_pintar(UI_LISTA);
    } else if (s_pantalla == UI_LISTA || s_pantalla == UI_LOCK) {
        // Vuelve al home con la misma sección seleccionada en el menú.
        for (int i = 0; i < MENU_N; i++) {
            if (FILTROS_MENU[i] == s_filtro) {
                s_menu = i;
                break;
            }
        }
        ui_pintar(UI_HOME);
    }
}

ui_pantalla_t ui_pantalla(void)
{
    return s_pantalla;
}
