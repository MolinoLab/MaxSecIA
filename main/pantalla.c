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

#define MAX_VENTANAS 4

static spi_device_handle_t s_spi = NULL;
static TaskHandle_t s_tarea = NULL;
static volatile bool s_ocupada = false;
static uint8_t *s_fb = NULL;
static uint8_t *s_prev = NULL;
static uint8_t *s_trozo = NULL;
static bool s_listo = false;
static bool s_completo = true;
static int s_vx0[MAX_VENTANAS];
static int s_vx1[MAX_VENTANAS];
static int s_nv = 0;

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

static void fijar_ventana(int x0, int x1)
{
    comando(0x44);
    dato(x0 & 0xFF);
    dato((x0 >> 8) & 0xFF);
    dato((x1 - 8) & 0xFF);
    dato(((x1 - 8) >> 8) & 0xFF);

    comando(0x45);
    dato((EPD_ALTO - 1) & 0xFF);
    dato((EPD_ALTO - 1) >> 8);
    dato(0x00);
    dato(0x00);

    comando(0x4E);
    dato(x0 & 0xFF);
    dato((x0 >> 8) & 0xFF);
    comando(0x4F);
    dato((EPD_ALTO - 1) & 0xFF);
    dato((EPD_ALTO - 1) >> 8);
}

static void escribir_plano(uint8_t cmd, int x0, int x1, const uint8_t *src)
{
    int wb = (x1 - x0) / 8;
    const int stride = EPD_ANCHO / 8;
    fijar_ventana(x0, x1);
    comando(cmd);
    for (int y = 0; y < EPD_ALTO; y++) {
        datos(src + (size_t)y * stride + (size_t)(x0 / 8), (size_t)wb);
    }
}

// Ventana a alto completo. Mismo modo de dirección que el refresco completo
// (X+, Y-): si no, el parcial queda en espejo respecto al frame base.
// El SSD1677 compara el plano 0x26 (lo que ya está en el panel) con el 0x24
// (lo nuevo). Si 0x26 se queda en un frame viejo, cada dos refrescos vuelve
// a salir la imagen anterior.
static bool refrescar_ventana(int x0, int x1)
{
    if (x0 < 0) {
        x0 = 0;
    }
    if (x1 > EPD_ANCHO) {
        x1 = EPD_ANCHO;
    }
    x0 &= ~7;
    x1 = (x1 + 7) & ~7;
    if (x1 <= x0) {
        return true;
    }

    comando(0x18);
    dato(0x80);
    comando(0x3C);
    dato(0x80);
    comando(0x11);
    dato(0x01);  // igual que iniciar_panel: X+, Y-
    comando(0x21);
    dato(0x00);
    dato(0x00);

    escribir_plano(0x26, x0, x1, s_prev);
    escribir_plano(0x24, x0, x1, s_fb);

    comando(0x22);
    dato(0xFF);
    comando(0x20);
    if (!esperar_libre()) {
        return false;
    }
    escribir_plano(0x26, x0, x1, s_fb);
    escribir_plano(0x24, x0, x1, s_fb);
    return true;
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

static void planear_ventanas(void)
{
    s_nv = 0;
    // Solo el primer frame (o un fallo previo) hace refresco completo.
    s_completo = !s_listo || !s_prev;
    if (s_completo) {
        return;
    }
    const int stride = EPD_ANCHO / 8;
    int b0 = -1;
    int b1 = -1;
    for (int b = 0; b < stride; b++) {
        bool dirty = false;
        for (int y = 0; y < EPD_ALTO; y++) {
            if (s_fb[(size_t)y * stride + b] != s_prev[(size_t)y * stride + b]) {
                dirty = true;
                break;
            }
        }
        if (dirty) {
            if (b0 < 0) {
                b0 = b;
            }
            b1 = b + 1;
        }
    }
    if (b0 < 0) {
        return;
    }
    // Una sola ventana que cubre todo lo cambiado: waveform parcial, sin SWRESET.
    s_vx0[0] = b0 * 8;
    s_vx1[0] = b1 * 8;
    s_nv = 1;
}

static void tarea_pantalla(void *arg)
{
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        bool ok = false;
        if (s_completo || !s_listo) {
            ESP_LOGI(TAG, "Refresco completo %dx%d", EPD_ANCHO, EPD_ALTO);
            if (iniciar_panel()) {
                volcar_y_refrescar();
                ok = true;
            }
        } else {
            ESP_LOGI(TAG, "Refresco parcial (%d ventanas)", s_nv);
            ok = true;
            for (int i = 0; i < s_nv && ok; i++) {
                ok = refrescar_ventana(s_vx0[i], s_vx1[i]);
            }
        }
        if (ok && s_prev) {
            memcpy(s_prev, s_fb, EPD_BYTES);
            s_listo = true;
        } else {
            s_listo = false;
        }
        s_ocupada = false;
    }
}

void pantalla_iniciar(void)
{
    s_fb = heap_caps_malloc(EPD_BYTES, MALLOC_CAP_SPIRAM);
    s_prev = heap_caps_malloc(EPD_BYTES, MALLOC_CAP_SPIRAM);
    s_trozo = heap_caps_malloc(TROZO_SPI, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!s_fb || !s_prev || !s_trozo) {
        ESP_LOGE(TAG, "Sin memoria para el framebuffer (%d bytes)", EPD_BYTES);
        return;
    }
    memset(s_prev, 0xFF, EPD_BYTES);
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

static bool encolar(const uint8_t *fb, bool forzar)
{
    if (!s_tarea || !s_fb || !s_prev || !fb) {
        return false;
    }
    if (!forzar && s_listo && memcmp(s_prev, fb, EPD_BYTES) == 0) {
        return false;
    }
    for (int i = 0; i < 1000 && s_ocupada; i++) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    if (s_ocupada) {
        ESP_LOGW(TAG, "Pantalla ocupada; se omite este frame");
        return false;
    }
    memcpy(s_fb, fb, EPD_BYTES);
    if (forzar) {
        s_completo = true;
        s_nv = 0;
    } else {
        planear_ventanas();
        if (!s_completo && s_nv == 0) {
            return false;
        }
    }
    s_ocupada = true;
    xTaskNotifyGive(s_tarea);
    return true;
}

bool pantalla_mostrar(const uint8_t *fb)
{
    return encolar(fb, true);
}

bool pantalla_actualizar(const uint8_t *fb)
{
    return encolar(fb, false);
}

bool pantalla_refresco_parcial(int x, int y, int w, int h, const uint8_t *fb)
{
    (void)y;
    (void)h;
    if (!fb) {
        return false;
    }
    // La UI compara el frame entero. Esta entrada fuerza la franja pedida
    // solo si el llamador ya dejó ese frame como actual.
    if (!encolar(fb, false)) {
        return false;
    }
    (void)x;
    (void)w;
    return true;
}

bool pantalla_ocupada(void)
{
    return s_ocupada;
}

void pantalla_dormir(void)
{
    if (!s_spi || !s_trozo) {
        return;
    }
    for (int i = 0; i < 1000 && s_ocupada; i++) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    comando(0x10);
    dato(0x01);
    vTaskDelay(pdMS_TO_TICKS(100));
    s_listo = false;
}
