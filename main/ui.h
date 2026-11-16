#pragma once

#include <stdbool.h>

typedef enum {
    UI_HOME,
    UI_LISTA,
    UI_DETALLE,
    UI_LOCK,
} ui_pantalla_t;

void ui_cargar_cache(void);
int ui_num_items(void);
void ui_pintar(ui_pantalla_t pantalla);
void ui_repintar(void);
void ui_mover(int delta);
void ui_abrir(void);
void ui_atras(void);
ui_pantalla_t ui_pantalla(void);
