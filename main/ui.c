// UI en portrait 480x800 (estilo FolloUp/Chroninkle: menú vertical, iconos, lock).
// Se rota a 800x480 al volcar. No usa código GPLv3 de Chroninkle.
#include "ui.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "almacen.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "pantalla.h"
#include "placa.h"

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
    FILTRO_AGENDA,
    FILTRO_TODO,
} filtro_t;

static item_t s_items[MAX_ITEMS];
static int s_n = 0;
static int s_sel = 0;
static int s_menu = 0;
static filtro_t s_filtro = FILTRO_TODOS;
static int s_vista[MAX_ITEMS];
static int s_nvista = 0;
static ui_pantalla_t s_pantalla = UI_HOME;
static uint8_t *s_logic = NULL;
static uint8_t *s_fb = NULL;

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
};
_Static_assert(sizeof(FONT) / sizeof(FONT[0]) == (size_t)('Z' - GLIFO_0 + 1), "fuente");

static void px(int x, int y, bool negro)
{
    if (x < 0 || y < 0 || x >= UI_W || y >= UI_H) {
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

static void texto_esc(int x, int y, const char *s, int esc)
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
    texto_esc(x, y, s, 3);
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
    } else {
        icono_sticky(x, y);
    }
}

static bool pasa_filtro(const item_t *it, filtro_t f)
{
    switch (f) {
    case FILTRO_FOLLOW:
        return strcmp(it->tipo, "deck") == 0;
    case FILTRO_TODOS:
        return strcmp(it->tipo, "deck") == 0 || strcmp(it->tipo, "pendiente") == 0;
    case FILTRO_NOTAS:
        return strcmp(it->tipo, "nota") == 0 || strcmp(it->tipo, "idea") == 0;
    case FILTRO_AGENDA:
        return strcmp(it->tipo, "evento") == 0;
    case FILTRO_TODO:
    default:
        return true;
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

static void reconstruir_vista(void)
{
    s_nvista = 0;
    for (int i = 0; i < s_n; i++) {
        if (pasa_filtro(&s_items[i], s_filtro)) {
            s_vista[s_nvista++] = i;
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

// Portrait (lx,ly) -> landscape físico 800x480, rotación 90° CW.
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
    pantalla_mostrar(s_fb);
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

static void pie_ayuda(const char *msg)
{
    hline(MARGEN, UI_H - 56, UI_W - 2 * MARGEN, true);
    texto(MARGEN, UI_H - 40, msg);
}

static void pintar_home(void)
{
    limpiar();
    titulo(MARGEN, 28, "MaxSecIA");

    int n_todos = contar_filtro(FILTRO_TODOS);
    char line[48];
    if (n_todos == 0) {
        texto(MARGEN, 90, "Sin tareas pendientes");
        barra_progreso(MARGEN, 120, UI_W - 2 * MARGEN, 18, 0);
    } else {
        snprintf(line, sizeof(line), "%d pendientes", n_todos);
        texto(MARGEN, 90, line);
        // Sin "hechos" en el resumen: barra llena según carga (tope visual).
        int pct = n_todos > 12 ? 100 : (n_todos * 100) / 12;
        barra_progreso(MARGEN, 120, UI_W - 2 * MARGEN, 18, pct);
    }

    time_t now = time(NULL);
    struct tm tm;
    if (now > 100000 && localtime_r(&now, &tm)) {
        static const char *DIAS[] = {"Dom", "Lun", "Mar", "Mie", "Jue", "Vie", "Sab"};
        snprintf(line, sizeof(line), "%s %02d/%02d", DIAS[tm.tm_wday], tm.tm_mday, tm.tm_mon + 1);
        texto(MARGEN, 160, line);
    } else {
        texto(MARGEN, 160, "Captura una idea al vuelo");
    }

    static const char *NOMBRES[MENU_N] = {"Follow up", "Todos", "Notas", "Agenda", "Todo"};
    static const filtro_t FILTROS[MENU_N] = {FILTRO_FOLLOW, FILTRO_TODOS, FILTRO_NOTAS, FILTRO_AGENDA, FILTRO_TODO};

    int y0 = 210;
    for (int i = 0; i < MENU_N; i++) {
        int y = y0 + i * 88;
        bool sel = (i == s_menu);
        rect(MARGEN - 8, y - 10, UI_W - 2 * MARGEN + 16, 76, false, true);
        if (sel) {
            rect(MARGEN - 4, y - 6, UI_W - 2 * MARGEN + 8, 68, false, true);
            rect(MARGEN - 8, y - 10, 10, 76, true, true);
        }
        int ix = MARGEN + 16;
        int iy = y + 12;
        switch (i) {
        case 0:
            icono_sticky(ix, iy);
            break;
        case 1:
            icono_check(ix, iy);
            break;
        case 2:
            icono_nota(ix, iy);
            break;
        case 3:
            icono_cal(ix, iy);
            break;
        default:
            icono_lista(ix, iy);
            break;
        }
        titulo(MARGEN + 64, y + 12, NOMBRES[i]);
        snprintf(line, sizeof(line), "%d", contar_filtro(FILTROS[i]));
        texto(UI_W - MARGEN - 56, y + 20, line);
        texto(UI_W - MARGEN - 24, y + 20, ">");
    }
    pie_ayuda("Select abre  ·  Record graba  ·  PWR bloquea");
    volcar_rotado();
}

static void pintar_lista(void)
{
    limpiar();
    static const char *TIT[MENU_N] = {"Follow up", "Todos", "Notas", "Agenda", "Todo"};
    titulo(MARGEN, 28, TIT[s_filtro]);
    char badge[24];
    snprintf(badge, sizeof(badge), "%d items", s_nvista);
    texto(MARGEN, 78, badge);
    hline(MARGEN, 110, UI_W - 2 * MARGEN, true);

    if (s_nvista == 0) {
        texto(MARGEN, 200, "Nada en esta lista");
        pie_ayuda("BOOT atras");
        volcar_rotado();
        return;
    }

    int base = s_sel - 3;
    if (base < 0) {
        base = 0;
    }
    int y = 130;
    for (int row = 0; row < 7 && base + row < s_nvista; row++) {
        int vi = base + row;
        int ii = s_vista[vi];
        bool sel = (vi == s_sel);
        int yh = 78;
        if (sel) {
            rect(MARGEN - 6, y - 6, UI_W - 2 * MARGEN + 12, yh, false, true);
            rect(MARGEN - 4, y - 4, UI_W - 2 * MARGEN + 8, yh - 4, false, true);
        } else {
            rect(MARGEN - 4, y - 4, UI_W - 2 * MARGEN + 8, yh - 4, false, true);
        }
        icono_tipo(s_items[ii].tipo, MARGEN + 8, y + 12);
        // título truncado
        char t[40];
        snprintf(t, sizeof(t), "%s", s_items[ii].titulo);
        texto(MARGEN + 52, y + 8, t);
        if (s_items[ii].fecha[0]) {
            texto(MARGEN + 52, y + 40, s_items[ii].fecha);
        } else {
            texto(MARGEN + 52, y + 40, s_items[ii].tipo);
        }
        y += yh + 4;
    }
    pie_ayuda("Rocker mueve  ·  Select detalle  ·  BOOT atras");
    volcar_rotado();
}

static void pintar_detalle(void)
{
    limpiar();
    if (s_nvista == 0) {
        texto(MARGEN, 40, "Sin elemento");
        volcar_rotado();
        return;
    }
    int ii = s_vista[s_sel];
    // sticky
    rect(MARGEN, 40, UI_W - 2 * MARGEN, UI_H - 140, false, true);
    rect(MARGEN + 4, 44, UI_W - 2 * MARGEN - 8, UI_H - 148, false, true);
    icono_tipo(s_items[ii].tipo, MARGEN + 24, 70);
    titulo(MARGEN + 70, 78, s_items[ii].tipo);
    if (s_items[ii].fecha[0]) {
        texto(MARGEN + 24, 130, s_items[ii].fecha);
    }
    hline(MARGEN + 24, 170, UI_W - 2 * MARGEN - 48, true);

    // título multilínea simple
    const char *p = s_items[ii].titulo;
    int y = 200;
    char linea[28];
    while (*p && y < UI_H - 160) {
        int n = 0;
        while (p[n] && n < (int)sizeof(linea) - 1) {
            n++;
        }
        // cortar por espacios cerca del final
        int cut = n;
        if (p[n]) {
            for (int k = n; k > 12; k--) {
                if (p[k] == ' ') {
                    cut = k;
                    break;
                }
            }
        }
        memcpy(linea, p, cut);
        linea[cut] = '\0';
        titulo(MARGEN + 24, y, linea);
        p += cut;
        while (*p == ' ') {
            p++;
        }
        y += 40;
    }
    pie_ayuda("BOOT atras");
    volcar_rotado();
}

static void pintar_lock(void)
{
    limpiar();
    time_t now = time(NULL);
    struct tm tm;
    char line[48];
    if (now > 100000 && localtime_r(&now, &tm)) {
        static const char *DIAS[] = {"Domingo", "Lunes", "Martes", "Miercoles", "Jueves", "Viernes", "Sabado"};
        snprintf(line, sizeof(line), "%s %02d/%02d", DIAS[tm.tm_wday], tm.tm_mday, tm.tm_mon + 1);
        texto(MARGEN, 40, line);
    } else {
        texto(MARGEN, 40, "MaxSecIA");
    }

    int n = contar_filtro(FILTRO_TODOS);
    if (n == 0) {
        titulo(MARGEN, 120, "TO-DO");
        texto(MARGEN, 200, "Todo al dia");
    } else {
        snprintf(line, sizeof(line), "TO-DO - %d PENDING", n);
        titulo(MARGEN, 100, line);
        hline(MARGEN, 150, UI_W - 2 * MARGEN, true);
        int y = 180;
        int shown = 0;
        for (int i = 0; i < s_n && shown < 5; i++) {
            if (!pasa_filtro(&s_items[i], FILTRO_TODOS)) {
                continue;
            }
            rect(MARGEN, y, UI_W - 2 * MARGEN, 72, false, true);
            icono_check(MARGEN + 12, y + 20);
            char t[36];
            snprintf(t, sizeof(t), "%s", s_items[i].titulo);
            texto(MARGEN + 56, y + 24, t);
            y += 84;
            shown++;
        }
        if (n > 5) {
            snprintf(line, sizeof(line), "+%d more pending", n - 5);
            texto(MARGEN, y + 8, line);
        }
    }
    texto(MARGEN, UI_H - 40, "PWR desbloquea");
    volcar_rotado();
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

void ui_mover(int delta)
{
    if (s_pantalla == UI_HOME) {
        s_menu += delta;
        if (s_menu < 0) {
            s_menu = 0;
        }
        if (s_menu >= MENU_N) {
            s_menu = MENU_N - 1;
        }
        ui_pintar(UI_HOME);
        return;
    }
    if (s_pantalla == UI_LISTA && s_nvista > 0) {
        s_sel += delta;
        if (s_sel < 0) {
            s_sel = 0;
        }
        if (s_sel >= s_nvista) {
            s_sel = s_nvista - 1;
        }
        ui_pintar(UI_LISTA);
    }
}

void ui_abrir(void)
{
    if (s_pantalla == UI_HOME) {
        static const filtro_t FILTROS[MENU_N] = {FILTRO_FOLLOW, FILTRO_TODOS, FILTRO_NOTAS, FILTRO_AGENDA, FILTRO_TODO};
        s_filtro = FILTROS[s_menu];
        s_sel = 0;
        reconstruir_vista();
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
        ui_pintar(UI_HOME);
    }
}

ui_pantalla_t ui_pantalla(void)
{
    return s_pantalla;
}
