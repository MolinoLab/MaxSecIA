// Pines de la Waveshare ESP32-S3-ePaper-3.97.
// Fuentes públicas: config de la placa en xiaozhi-esp32 y el mapa de wave-reader
// (CLK/CMD/datos de la SD, rocker). No es código de Chroninkle.
#pragma once

#include "driver/gpio.h"

// La alimentación la lleva el AXP2101 (I2C 0x34). No hay raíl GPIO de pantalla ni de audio.
#define PIN_BOTON_BOOT   GPIO_NUM_0   // Grabar / sincronizar (como MiniSecIA)
#define PIN_BOTON_PWR    GPIO_NUM_1   // Cola / apagar (activo a nivel bajo)
#define PIN_ROCKER_ARRIBA GPIO_NUM_4
#define PIN_ROCKER_SELECT GPIO_NUM_5
#define PIN_ROCKER_ABAJO  GPIO_NUM_6

// I2C: ES8311 (0x18), AXP2101 (0x34), PCF85063 (0x51), SHTC3, QMI8658
#define PIN_I2C_SDA      GPIO_NUM_41
#define PIN_I2C_SCL      GPIO_NUM_42
#define DIR_RTC          0x51
#define DIR_PMIC         0x34

// I2S hacia el ES8311
#define PIN_I2S_MCLK     GPIO_NUM_13
#define PIN_I2S_BCLK     GPIO_NUM_14
#define PIN_I2S_WS       GPIO_NUM_47
#define PIN_I2S_DOUT     GPIO_NUM_48
#define PIN_I2S_DIN      GPIO_NUM_21
#define PIN_PA           GPIO_NUM_39

// e-Paper 3,97" 800x480 B/N (SSD1677) por SPI3
#define PIN_EPD_DC       GPIO_NUM_9
#define PIN_EPD_CS       GPIO_NUM_10
#define PIN_EPD_SCK      GPIO_NUM_11
#define PIN_EPD_MOSI     GPIO_NUM_12
#define PIN_EPD_RST      GPIO_NUM_46
#define PIN_EPD_BUSY     GPIO_NUM_3   // 1 = ocupada

// microSD SDMMC 4 bits (si falla, almacen.c prueba 1 bit)
#define PIN_SD_CLK       GPIO_NUM_16
#define PIN_SD_CMD       GPIO_NUM_17
#define PIN_SD_D0        GPIO_NUM_15
#define PIN_SD_D1        GPIO_NUM_7
#define PIN_SD_D2        GPIO_NUM_8
#define PIN_SD_D3        GPIO_NUM_18

#define EPD_ANCHO        800
#define EPD_ALTO         480
#define EPD_BYTES        (EPD_ANCHO * EPD_ALTO / 8)  // 1 bit/píxel, 48000

#define AUDIO_HZ         16000
#define PUNTO_MONTAJE    "/sdcard"
