#include "energia.h"

#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "placa.h"

static const char *TAG = "energia";
static volatile modo_led_t s_modo_led = LED_APAGADO;
static i2c_master_dev_handle_t s_pmic = NULL;

static bool pmic_escribir(uint8_t reg, uint8_t val)
{
    if (!s_pmic) {
        return false;
    }
    uint8_t buf[2] = {reg, val};
    return i2c_master_transmit(s_pmic, buf, sizeof(buf), 100) == ESP_OK;
}

void energia_pmic_iniciar(i2c_master_bus_handle_t bus)
{
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DIR_PMIC,
        .scl_speed_hz = 100000,
    };
    if (i2c_master_bus_add_device(bus, &cfg, &s_pmic) != ESP_OK) {
        ESP_LOGW(TAG, "No hay AXP2101 en el bus I2C");
        s_pmic = NULL;
        return;
    }
    // Secuencia publicada para esta placa (xiaozhi / Waveshare 3.97): ALDO1–3 a 3,3 V.
    pmic_escribir(0x22, 0x06);
    pmic_escribir(0x27, 0x10);
    pmic_escribir(0x80, 0x01);
    pmic_escribir(0x90, 0x00);
    pmic_escribir(0x91, 0x00);
    pmic_escribir(0x82, (3300 - 1500) / 100);
    pmic_escribir(0x92, (3300 - 500) / 100);
    pmic_escribir(0x93, (3300 - 500) / 100);
    pmic_escribir(0x94, (3300 - 500) / 100);
    pmic_escribir(0x90, 0x07);
    pmic_escribir(0x64, 0x03);
    pmic_escribir(0x61, 0x02);
    pmic_escribir(0x62, 0x08);
    pmic_escribir(0x63, 0x01);
    // La microSD y el panel necesitan que los ALDO estabilicen (ejemplo Waveshare: ~1 s).
    vTaskDelay(pdMS_TO_TICKS(800));
    ESP_LOGI(TAG, "AXP2101: ALDO1-3 a 3,3 V");
}

// La 3.97 no tiene LED de estado ni raíles GPIO como la 1.54G.
// El apagado real de la batería lo hace el AXP2101; aquí solo hay sueño profundo.

despertar_t energia_iniciar(void)
{
    despertar_t causa = DESPERTAR_ENCENDIDO;
    esp_sleep_wakeup_cause_t c = esp_sleep_get_wakeup_cause();
    if (c == ESP_SLEEP_WAKEUP_EXT1) {
        uint64_t pines = esp_sleep_get_ext1_wakeup_status();
        causa = (pines & (1ULL << PIN_BOTON_BOOT)) ? DESPERTAR_BOOT : DESPERTAR_PWR;
    } else if (c == ESP_SLEEP_WAKEUP_TIMER) {
        causa = DESPERTAR_TEMPORIZADOR;
    }
    if (c != ESP_SLEEP_WAKEUP_UNDEFINED) {
        rtc_gpio_deinit(PIN_BOTON_BOOT);
        rtc_gpio_deinit(PIN_BOTON_PWR);
    }
    return causa;
}

void energia_audio(bool encendido)
{
    (void)encendido;
}

void energia_pantalla(bool encendido)
{
    (void)encendido;
}

void energia_led(modo_led_t modo)
{
    s_modo_led = modo;
    ESP_LOGD(TAG, "LED %d (sin GPIO en esta placa)", (int)modo);
}

int energia_bateria_mv(void)
{
    // La tensión sale del AXP2101; aún no se lee por I2C.
    return 0;
}

static void esperar_botones_sueltos(void)
{
    for (int i = 0; i < 1000; i++) {
        if (gpio_get_level(PIN_BOTON_BOOT) && gpio_get_level(PIN_BOTON_PWR)) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelay(pdMS_TO_TICKS(50));
}

static void dormir_con_botones(uint32_t segundos)
{
    esperar_botones_sueltos();
    const uint64_t botones = (1ULL << PIN_BOTON_BOOT) | (1ULL << PIN_BOTON_PWR);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    rtc_gpio_pullup_en(PIN_BOTON_BOOT);
    rtc_gpio_pulldown_dis(PIN_BOTON_BOOT);
    rtc_gpio_pullup_en(PIN_BOTON_PWR);
    rtc_gpio_pulldown_dis(PIN_BOTON_PWR);
    esp_sleep_enable_ext1_wakeup_io(botones, ESP_EXT1_WAKEUP_ANY_LOW);
    if (segundos > 0) {
        esp_sleep_enable_timer_wakeup((uint64_t)segundos * 1000000ULL);
    }
    esp_deep_sleep_start();
}

void energia_dormir(uint32_t segundos)
{
    ESP_LOGI(TAG, "Sueño profundo (%lu s)", (unsigned long)segundos);
    s_modo_led = LED_APAGADO;
    dormir_con_botones(segundos);
}

void energia_apagar(void)
{
    ESP_LOGW(TAG, "Sin corte de PMIC: sueño profundo sin temporizador");
    s_modo_led = LED_APAGADO;
    dormir_con_botones(0);
}
