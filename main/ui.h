#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    UI_HOME,
    UI_LISTA,
    UI_DETALLE,
    UI_ACCION,
    UI_LOCK,
} ui_pantalla_t;

typedef enum {
    UI_SIGUE = 0,
    UI_PEDIR_COMPLETAR,
    UI_PEDIR_ARCHIVAR,
} ui_paso_t;

void ui_cargar_cache(void);
int ui_num_items(void);
void ui_pintar(ui_pantalla_t pantalla);
void ui_repintar(void);
void ui_mover(int delta);
ui_paso_t ui_abrir(void);
void ui_atras(void);
ui_pantalla_t ui_pantalla(void);

// Copia tipo e id de la tarea abierta en el menú de acción.
bool ui_accion_datos(char *tipo, size_t ntipo, char *id, size_t nid);
