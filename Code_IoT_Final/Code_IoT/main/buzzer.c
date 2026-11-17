#include "buzzer.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BUZZER";

esp_err_t buzzer_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUZZER_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = (BUZZER_ACTIVE_LEVEL == 0) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = (BUZZER_ACTIVE_LEVEL == 1) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    esp_err_t err = gpio_config(&io_conf);
    if (err == ESP_OK) {
        buzzer_set(false); // Start with buzzer OFF
        ESP_LOGI(TAG, "Buzzer GPIO%d initialized successfully (Active %s)", 
                 BUZZER_GPIO, (BUZZER_ACTIVE_LEVEL == 1) ? "HIGH" : "LOW");
    } else {
        ESP_LOGE(TAG, "Failed to initialize Buzzer GPIO: %s", esp_err_to_name(err));
    }
    return err;
}

void buzzer_set(bool active)
{
    int level = active ? BUZZER_ACTIVE_LEVEL : (!BUZZER_ACTIVE_LEVEL);
    gpio_set_level(BUZZER_GPIO, level);
}

void buzzer_beep(uint32_t duration_ms)
{
    buzzer_set(true);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    buzzer_set(false);
}
