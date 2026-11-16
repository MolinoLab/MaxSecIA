// e-Paper 3,97" 800x480 monocromo (SSD1677).
// Secuencia alineada con el ejemplo oficial Waveshare EPD_3in97 (Init + Display_Base).
#include "pantalla.h"

#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "pantalla";

#define TROZO_SPI 4096
#define BUSY_TIMEOUT_MS 20000

static spi_device_handle_t s_spi = NULL;
static TaskHandle_t s_tarea = NULL;
static volatile bool s_ocupada = false;
static uint8_t *s_fb = NULL;
static uint8_t *s_trozo = NULL;

static void enviar(const uint8_t *datos, size_t n)
{
    spi_transaction_t t = {0};
    t.length = n * 8;
    t.tx_buffer = datos;
    spi_device_polling_transmit(s_spi, &t);
}

static void comando(uint8_t c)
{
    gpio_set_level(PIN_EPD_DC, 0);
    gpio_set_level(PIN_EPD_CS, 0);
    s_trozo[0] = c;
    enviar(s_trozo, 1);
    gpio_set_level(PIN_EPD_CS, 1);
}

static void dato(uint8_t d)
{
    gpio_set_level(PIN_EPD_DC, 1);
    gpio_set_level(PIN_EPD_CS, 0);
    s_trozo[0] = d;
    enviar(s_trozo, 1);
    gpio_set_level(PIN_EPD_CS, 1);
}

static void datos(const uint8_t *d, size_t n)
{
    gpio_set_level(PIN_EPD_DC, 1);
    gpio_set_level(PIN_EPD_CS, 0);
    while (n > 0) {
        size_t k = n < TROZO_SPI ? n : TROZO_SPI;
        memcpy(s_trozo, d, k);
        enviar(s_trozo, k);
        d += k;
        n -= k;
    }
    gpio_set_level(PIN_EPD_CS, 1);
}

// Waveshare: BUSY en alto mientras refresca; libre en bajo.
static bool esperar_libre(void)
{
    for (int ms = 0; ms < BUSY_TIMEOUT_MS; ms += 20) {
        if (gpio_get_level(PIN_EPD_BUSY) == 0) {
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    ESP_LOGE(TAG, "La pantalla no responde (BUSY)");
    return false;
}

static void reset_hw(void)
{
    gpio_set_level(PIN_EPD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(PIN_EPD_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(2));
    gpio_set_level(PIN_EPD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(20));
}

static bool iniciar_panel(void)
{
    reset_hw();
    if (!esperar_libre()) {
        return false;
    }
    comando(0x12);  // SWRESET
    if (!esperar_libre()) {
        return false;
    }

    comando(0x18);
    dato(0x80);

    comando(0x0C);
    dato(0xAE);
    dato(0xC7);
    dato(0xC3);
    dato(0xC0);
    dato(0x80);

    // Driver output: HEIGHT-1 = 479, SM=0x02
    comando(0x01);
    dato((EPD_ALTO - 1) & 0xFF);
    dato((EPD_ALTO - 1) >> 8);
    dato(0x02);

    comando(0x3C);
    dato(0x01);

    // X+, Y-
    comando(0x11);
    dato(0x01);

    // RAM X: 0 .. WIDTH-1 (en píxeles)
    comando(0x44);
    dato(0x00);
    dato(0x00);
    dato((EPD_ANCHO - 1) & 0xFF);
    dato((EPD_ANCHO - 1) >> 8);

    // RAM Y: HEIGHT-1 .. 0
    comando(0x45);
    dato((EPD_ALTO - 1) & 0xFF);
    dato((EPD_ALTO - 1) >> 8);
    dato(0x00);
    dato(0x00);

    comando(0x4E);
    dato(0x00);
    dato(0x00);
    comando(0x4F);
    dato(0x00);
    dato(0x00);
    return esperar_libre();
}

static void volcar_y_refrescar(void)
{
    // Display_Base: mismo frame en 0x24 (BW) y 0x26 (RED/previo).
    comando(0x24);
    datos(s_fb, EPD_BYTES);
    comando(0x26);
    datos(s_fb, EPD_BYTES);

    comando(0x22);
    dato(0xF7);
    comando(0x20);
    esperar_libre();
}

static void tarea_pantalla(void *arg)
{
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ESP_LOGI(TAG, "Refresco completo %dx%d", EPD_ANCHO, EPD_ALTO);
        if (iniciar_panel()) {
            volcar_y_refrescar();
        }
        s_ocupada = false;
    }
}

void pantalla_iniciar(void)
{
    s_fb = heap_caps_malloc(EPD_BYTES, MALLOC_CAP_SPIRAM);
    s_trozo = heap_caps_malloc(TROZO_SPI, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!s_fb || !s_trozo) {
        ESP_LOGE(TAG, "Sin memoria para el framebuffer (%d bytes)", EPD_BYTES);
        return;
    }
    spi_bus_config_t bus = {
        .miso_io_num = -1,
        .mosi_io_num = PIN_EPD_MOSI,
        .sclk_io_num = PIN_EPD_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = TROZO_SPI,
    };
    spi_device_interface_config_t dev = {
        .spics_io_num = -1,
        .clock_speed_hz = 10 * 1000 * 1000,
        .mode = 0,
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &bus, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI3_HOST, &dev, &s_spi));
    gpio_config_t sal = {
        .pin_bit_mask = (1ULL << PIN_EPD_RST) | (1ULL << PIN_EPD_DC) | (1ULL << PIN_EPD_CS),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&sal);
    gpio_set_level(PIN_EPD_CS, 1);
    gpio_set_level(PIN_EPD_RST, 1);
    gpio_config_t ent = {.pin_bit_mask = 1ULL << PIN_EPD_BUSY, .mode = GPIO_MODE_INPUT};
    gpio_config(&ent);
    xTaskCreate(tarea_pantalla, "pantalla", 4096, NULL, 3, &s_tarea);
}

bool pantalla_mostrar(const uint8_t *fb)
{
    if (!s_tarea || !s_fb || !fb) {
        return false;
    }
    memcpy(s_fb, fb, EPD_BYTES);
    s_ocupada = true;
    xTaskNotifyGive(s_tarea);
    return true;
}

bool pantalla_refresco_parcial(int x, int y, int w, int h, const uint8_t *fb)
{
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)fb;
    ESP_LOGW(TAG, "Refresco parcial no implementado; usa pantalla_mostrar()");
    return false;
}

bool pantalla_ocupada(void)
{
    return s_ocupada;
}
