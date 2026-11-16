# Qué se toma de FolloUp / Chroninkle

Referencias, solo como patrón de uso. Su código es GPLv3 (ALXV y mach1na) y
**no** se copia ni se adapta en este repositorio.

- [mach1na/chroninkle](https://github.com/mach1na/chroninkle)
- FolloUp (notas de voz en la misma familia de placas Waveshare 3.97")

## Patrones que sí se reimplementan

- Pantalla grande en vertical de uso: inicio con un resumen, lista, ficha de un
  elemento y una pantalla de bloqueo que deja los pendientes congelados en el
  e-Paper al dormir o apagar.
- Rocker de tres vías (arriba, centro, abajo) más Record y PWR.
- Record mantenido graba; al soltar se cierra el audio. PWR corto bloquea;
  PWR largo apaga.
- Los tipos de captura siguen el parseo de voz que ya tiene ConvocAUTOrias:
  tarea, idea, evento, nota o pendiente. En la lista aparecen como `deck`,
  `evento`, `pendiente`, `nota` e `idea`.

## Qué queda fuera de esta fase

Stickies, topics y books al estilo Chroninkle, marcar una tarjeta de Deck como
hecha desde el aparato, y el refresco parcial fino. Chroninkle manda el audio a
Gemini en el propio flujo del reloj; MaxSecIA no hace eso: el WAV sale por
`POST /voz`, igual que MiniSecIA.

El aparato no abre Nextcloud. Pide el resumen a ConvocAUTOrias y lo guarda en
`/sdcard/cache/resumen.txt`.
