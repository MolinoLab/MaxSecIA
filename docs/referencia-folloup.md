# Qué se toma de FolloUp / Chroninkle

Referencias, solo como patrón de uso. Su código es GPLv3 (ALXV y mach1na) y
**no** se copia ni se adapta en este repositorio.

- [mach1na/chroninkle](https://github.com/mach1na/chroninkle)
- FolloUp (notas de voz en la misma familia de placas Waveshare 3.97")

## Patrones que sí se reimplementan

- Canvas lógico en **portrait 480×800** (rotado al panel 800×480), como FolloUp.
- Home con barra de estado (hora, Wi-Fi, batería) y menú inferior:
  Follow up, Tareas, Notas, Topics y Books.
- Al mover el selector solo se refresca la franja que cambia. Si el contenido
  no cambió, no hay refresco. El descanso deja la imagen que ya está en el panel.
- Listas en filas con icono por tipo y texto en caja; detalle como “sticky”;
  lock con resumen de pendientes.
- Rocker: arriba/abajo navegan; clic corto entra; clic largo sale.
- BOOT y PWR como MiniSecIA: BOOT corto graba o para; BOOT largo sincroniza;
  PWR corto cancela o avisa de la cola; PWR largo apaga.
- Los tipos de captura siguen el parseo de voz que ya tiene ConvocAUTOrias:
  tarea, idea, evento, nota o pendiente. En la lista aparecen como `deck`,
  `evento`, `pendiente`, `nota` e `idea`. Follow = Deck + eventos; Tareas = Deck +
  pendientes. Books lee `.txt`/`.md` de `/sdcard/libros`. Topics aún vacío.

## Qué queda fuera de esta fase

Marcar una tarjeta de Deck como hecha desde el aparato, y topics generados en
el dispositivo. Chroninkle manda el audio a Gemini en el propio flujo del reloj;
MaxSecIA no hace eso: el WAV sale por `POST /voz`, igual que MiniSecIA.

El aparato no abre Nextcloud. Pide el resumen a ConvocAUTOrias y lo guarda en
`/sdcard/cache/resumen.txt`.
