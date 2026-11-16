# Pines: Waveshare ESP32-S3-ePaper-3.97

Placa con ESP32-S3, panel 800×480 blanco y negro (controlador tipo SSD1677),
códec ES8311, RTC PCF85063 y PMIC AXP2101. Flash 16 MB, PSRAM 8 MB octal.

El mapa sale de la documentación pública de la placa (esquema Waveshare y
`config.h` del ejemplo xiaozhi para `esp32-s3-epaper-3.97`) y del README del
proyecto wave-reader para la microSD y el rocker. No es código de FolloUp.

| Función | GPIO |
|---|---|
| BOOT | 0 |
| PWR | 1 |
| Rocker arriba | 4 |
| Rocker select | 5 |
| Rocker abajo | 6 |
| e-Paper BUSY (alto = ocupada) | 3 |
| e-Paper DC | 9 |
| e-Paper CS | 10 |
| e-Paper SCK (SPI3) | 11 |
| e-Paper MOSI | 12 |
| e-Paper RST | 46 |
| I2S MCLK | 13 |
| I2S BCLK | 14 |
| I2S DIN (mic) | 21 |
| PA | 39 |
| I2C SDA | 41 |
| I2C SCL | 42 |
| I2S WS | 47 |
| I2S DOUT | 48 |
| SD CLK | 16 |
| SD CMD | 17 |
| SD D0 | 15 |
| SD D1 | 7 |
| SD D2 | 8 |
| SD D3 | 18 |

I2C: RTC `0x51`, PMIC `0x34`.

Frente al 1.54" de MiniSecIA cambian el bus de la pantalla, la SD (SDMMC de 4 bits,
con reserva a 1 bit), el rocker y la energía. Aquí no hay LED de estado ni GPIO
que corte alimentación de pantalla o audio: eso lo hace el AXP2101. Apagar entra
en deep sleep; la lectura de batería en milivoltios aún devuelve 0 hasta cablear
el PMIC.

La imagen en RAM es 800×480 a 1 bit (48 000 bytes, bit 1 = blanco). El refresco
parcial del controlador está reservado en `pantalla_refresco_parcial` y de momento
no se usa: cada pintado es un refresco completo.
