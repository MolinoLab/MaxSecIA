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

static int pmic_leer(uint8_t reg)
{
    if (!s_pmic) {
        return -1;
    }
    uint8_t v = 0;
    if (i2c_master_transmit_receive(s_pmic, &reg, 1, &v, 1, 100) != ESP_OK) {
        return -1;
    }
    return v;
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
    // El apagado por mantener PWR no se arma aquí: el mismo pulsado que enciende
    // seguiría contando y el PMIC cortaría a los pocos segundos, a veces sin
    // volver a aceptar el botón. OFFLEVEL queda en 10 s para cuando se arme.
    pmic_escribir(0x22, 0x04);
    pmic_escribir(0x27, 0x1C);
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
    // Canal ADC de tensión de batería (bit 0 de 0x30).
    int adc = pmic_leer(0x30);
    if (adc >= 0) {
        pmic_escribir(0x30, (uint8_t)(adc | 0x01));
    }
    // Bit 0 de 0x10 es el corte inmediato. Bit 2 (PWR 16 s) se arma al soltar el botón.
    int comun = pmic_leer(0x10);
    if (comun >= 0) {
        pmic_escribir(0x10, (uint8_t)(comun & (uint8_t)~0x05));
    }
    // La microSD y el panel necesitan que los ALDO estabilicen (ejemplo Waveshare: ~1 s).
    vTaskDelay(pdMS_TO_TICKS(800));
    ESP_LOGI(TAG, "AXP2101: ALDO1-3 a 3,3 V");
}

// La 3.97 no tiene LED de estado ni raíles GPIO como la 1.54G.
// El apagado de la batería lo hace el AXP2101 (registro 0x10, bit 0).

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
    int hi = pmic_leer(0x34);
    int lo = pmic_leer(0x35);
    if (hi < 0 || lo < 0) {
        return 0;
    }
    int mv = ((hi & 0x3F) << 8) | lo;
    if (mv < 2500 || mv > 4500) {
        return 0;
    }
    return mv;
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

static bool s_apagado_hardware = false;

void energia_armar_apagado(void)
{
    if (s_apagado_hardware || !s_pmic) {
        return;
    }
    s_apagado_hardware = true;
    // Bit 1 de 0x22: PWR largo puede cortar si el firmware ya no responde.
    pmic_escribir(0x22, 0x06);
    int comun = pmic_leer(0x10);
    if (comun >= 0) {
        pmic_escribir(0x10, (uint8_t)((comun | 0x04) & (uint8_t)~0x01));
    }
    ESP_LOGI(TAG, "Apagado por PWR armado");
}

static bool hay_vbus(void)
{
    // STATUS1 bit 5: VBUS presente (XPowersLib / AXP2101).
    int st = pmic_leer(0x00);
    return st >= 0 && (st & 0x20) != 0;
}

void energia_apagar(void)
{
    s_modo_led = LED_APAGADO;
    esperar_botones_sueltos();
    // El PMIC tiene que ver PWR suelto antes del corte. Si se apaga con el
    // botón aún bajo, el siguiente pulsado no vuelve a encender.
    vTaskDelay(pdMS_TO_TICKS(250));
    if (s_pmic && !hay_vbus()) {
        ESP_LOGW(TAG, "Corte de batería (AXP2101)");
        int cfg = pmic_leer(0x10);
        if (cfg >= 0) {
            pmic_escribir(0x10, (uint8_t)(cfg | 0x01));
        }
        // Si el corte funciona, de aquí no se vuelve. Con el botón aún
        // interpretado como encendido, o si el registro no surte efecto, se duerme.
        vTaskDelay(pdMS_TO_TICKS(400));
        ESP_LOGW(TAG, "El PMIC no cortó la alimentación");
    } else {
        ESP_LOGW(TAG, "USB conectado: sueño profundo sin temporizador");
    }
    dormir_con_botones(0);
}
