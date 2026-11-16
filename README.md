# MaxSecIA

Grabadora de bolsillo grande: Waveshare ESP32-S3-ePaper 3.97" (800×480),
el mismo envío de voz que [MiniSecIA](https://github.com/MolinoLab/MiniSecIA)
y una lista de pendientes que sale de Nextcloud (Deck, Notes, CalDAV) y de
los almacenes locales de [ConvocAUTOrias](https://github.com/MolinoLab/ConvocAUTOrias).

El aparato no habla con Nextcloud. Sube el WAV a `POST /voz` y, cuando hay
red, baja `GET /resumen?formato=txt` a la microSD para poder pintar sin cobertura.

## Mandos

| Gesto | Efecto |
|---|---|
| BOOT mantenido | Empieza a grabar (pitido de inicio) |
| Soltar BOOT tras mantenerlo | Cierra el WAV y lo deja en la cola |
| BOOT corto | Cancela la grabación, o vuelve atrás en la lista |
| Rocker arriba / abajo | Mueve la selección (en la lista) |
| Rocker centro | Entra en la lista o abre el detalle |
| PWR corto | Congela el resumen de pendientes (bloqueo) o vuelve al inicio |
| PWR largo | Apaga: pinta el bloqueo y entra en deep sleep |

Los tonos son los de MiniSecIA: inicio, fin, error, batería, apagado y un
pitido por cada audio que sigue en cola al sincronizar.

## Pantallas

Inicio, lista, detalle y bloqueo. El bloqueo es lo que queda en el e-Paper
al dormir o apagar. El texto sale de `/sdcard/cache/resumen.txt`, una línea
por elemento: `tipo`, título, fecha e id, separados por tabulador.

## microSD

FAT32. En la raíz, `config.txt` (se importa a NVS y se borra):

```
url=https://voz.molinolab.org
token=EL_TOKEN_DE_43_CARACTERES
device_id=maxsecia
wifi=Nombre de la red,contraseña
```

Solo redes de 2,4 GHz. La cola de audio es `cola/` y `fallidos/`, igual que
en MiniSecIA. El resumen cacheado es `cache/resumen.txt`.

## Compilar

ESP-IDF 5.5 o posterior, destino `esp32s3`.

```
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

La UI completa se comprueba con la placa delante: el refresco del panel es
un primer puerto (completo, sin parcial) y la batería en milivoltios aún no
sale del AXP2101.

## Documentación

- [docs/pines-3.97.md](docs/pines-3.97.md) — pines y diferencias de hardware
- [docs/referencia-folloup.md](docs/referencia-folloup.md) — patrones de FolloUp, sin su código
- [docs/diferencias-minisecia.md](docs/diferencias-minisecia.md) — qué módulos se pueden copiar entre los dos

## Qué no hace todavía

Marcar tarjetas de Deck como hechas, notas adhesivas, refresco parcial y una
librería C compartida con MiniSecIA.
