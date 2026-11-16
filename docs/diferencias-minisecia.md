# MiniSecIA y MaxSecIA

Misma familia. Un cambio de protocolo (`POST /voz`, token Bearer, cabeceras
`X-Request-Id` y `X-Bateria-mV`, tonos, cola de WAV) se anota una vez y se
aplica en los dos repos. Todavía no hay carpeta `shared/`: el cliente se copió
y esta lista dice qué se puede volver a copiar.

## Se puede alinear a mano

| Módulo | Qué comparte |
|---|---|
| `cliente.c` | Subida multipart y `GET` de texto |
| `almacen.c` | Cola `cola/`, `fallidos/`, reparación de `.part` (la SD de Max es de 4 bits) |
| `audio.c` | ES8311, 16 kHz, tonos |
| `red.c` | Wi-Fi 2,4 GHz y `config.txt` |
| `ajustes.c` | NVS: URL, token, `device_id`, reposo |
| `reloj.c` | PCF85063 |

## Solo de esta placa

| Módulo | Por qué |
|---|---|
| `placa.h` | Pines del 3.97" |
| `pantalla.c` | 800×480, 1 bit, no las imágenes de 200×200 a 4 colores |
| `botones.c` | Rocker y grabar manteniendo BOOT |
| `energia.c` | Sin ADC en GPIO4 (ahí está el rocker); deep sleep, PMIC pendiente |
| `ui.c` / `contenido.c` | Lista híbrida y caché; MiniSecIA sigue con el carrusel de imágenes |

`imagenes.c` quedó en el árbol como referencia del 1.54" y **no** entra en el
`CMakeLists.txt` de MaxSecIA. Los PNG de `Imagenes pantalla/` son los del
bolsillo; esta pantalla no los muestra.

## Checklist al cambiar la voz o la API

1. Mismo camino y mismos códigos HTTP en `cliente.c` de Mini y de Max.
2. Si el resumen cambia de formato, subir `VERSION` en
   `ConvocAUTOrias/src/voz/resumen_dispositivo.py` y el comentario de `ui.c`.
3. No subir `config.txt` ni tokens.
