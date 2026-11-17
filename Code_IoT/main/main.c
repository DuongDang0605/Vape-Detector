#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

// Project headers
#include "i2c_bus.h"
#include "sht3x.h"
#include "sgp41.h"
#include "pms7003.h"
#include "vape_logic.h"
#include "oled_ssd1306.h"
#include "buzzer.h"
#include "ble_app.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "===============================================");
    ESP_LOGI(TAG, "      ESP32-S3 VAPE DETECTOR STARTING          ");
    ESP_LOGI(TAG, "===============================================");

    // 1. Initialize logic states
    vape_logic_init();

    // 2. Initialize hardware buses
    esp_err_t err = i2c_bus_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C master bus, halting!");
        return;
    }

    err = pms7003_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize PMS7003 UART, halting!");
        return;
    }

    // Initialize OLED screen
    err = oled_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "OLED screen init failed, continuing without display");
    }

    // Initialize Buzzer
    err = buzzer_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Buzzer init failed");
    } else {
        // Startup confirmation: beep for exactly 1 second, then shut off
        ESP_LOGI(TAG, "Buzzer startup test: beeping for 1 second...");
        buzzer_beep(1000);
        buzzer_set(false);
    }

    // Initialize BLE NimBLE GATT Server
    err = ble_app_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "BLE init failed");
    }

    // Give hardware buses a moment to settle
    vTaskDelay(pdMS_TO_TICKS(100));

    // 3. Initialize sensors
    err = sht3x_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "SHT3x init failed, will attempt to read anyway");
    }

    err = sgp41_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "SGP41 init failed, will attempt to read anyway");
    }

    // 4. Run SGP41 10-Second Conditioning Phase
    ESP_LOGI(TAG, "Starting 10-second SGP41 sensor conditioning phase...");
    float pm25_sum = 0.0f;
    int pm25_valid_count = 0;
    pms7003_data_t cond_pm_data = {0};

    for (int i = 1; i <= 10; i++) {
        float temp = 25.0f, humid = 50.0f;
        uint16_t raw_temp = SGP41_DEFAULT_T_TICKS;
        uint16_t raw_humid = SGP41_DEFAULT_RH_TICKS;
        uint16_t sraw_voc = 0;

        // Try to read actual environment to compensate the conditioning hotplate
        if (sht3x_read_temp_humid(&temp, &humid, &raw_temp, &raw_humid) == ESP_OK) {
            ESP_LOGI(TAG, "Conditioning [%d/10] - Compensating with T: %.1fC, RH: %.1f%%", i, temp, humid);
        } else {
            ESP_LOGW(TAG, "Conditioning [%d/10] - SHT3x read error, using default compensation", i);
        }

        // Execute conditioning step
        if (sgp41_execute_conditioning(raw_temp, raw_humid, &sraw_voc) == ESP_OK) {
            ESP_LOGI(TAG, "Conditioning [%d/10] - SGP41 SRAW_VOC: %d ticks", i, sraw_voc);
        } else {
            ESP_LOGE(TAG, "Conditioning [%d/10] - SGP41 conditioning command failed!", i);
        }

        // Read PMS7003 to clear buffer and build dynamic baseline
        if (pms7003_read(&cond_pm_data) == ESP_OK) {
            // Apply humidity compensation for baseline too
            float rh_ratio = humid / 100.0f;
            float compensation = 1.0f + 0.25f * (rh_ratio * rh_ratio);
            uint16_t cond_pm25_final = (uint16_t)((float)cond_pm_data.pm2_5_atm / compensation);
            
            pm25_sum += cond_pm25_final;
            pm25_valid_count++;
        }

        // Display conditioning status on OLED
        oled_clear();
        oled_draw_string(0, 0, "=== CALIBRATION ===");
        char oled_cond_str[24];
        snprintf(oled_cond_str, sizeof(oled_cond_str), "Step: %d / 10", i);
        oled_draw_string(0, 2, oled_cond_str);
        snprintf(oled_cond_str, sizeof(oled_cond_str), "T:%.1fC  RH:%.1f%%", temp, humid);
        oled_draw_string(0, 4, oled_cond_str);
        oled_draw_string(0, 6, "Sensing hotplate...");
        oled_update();

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    if (pm25_valid_count > 0) {
        vape_logic_set_pm25_baseline(pm25_sum / pm25_valid_count);
    } else {
        vape_logic_set_pm25_baseline(36.0f); // Fallback if sensor fails to read
    }

    ESP_LOGI(TAG, "SGP41 conditioning completed. Starting active monitoring loop...");

    // 5. Active Monitoring Loop
    pms7003_data_t pm_data = {0};
    bool pm_valid = false;

    while (1) {
        TickType_t start_tick = xTaskGetTickCount();

        float temp = 25.0f;
        float humid = 50.0f;
        uint16_t raw_temp = SGP41_DEFAULT_T_TICKS;
        uint16_t raw_humid = SGP41_DEFAULT_RH_TICKS;
        uint16_t sraw_voc = 0;
        uint16_t sraw_nox = 0;

        // Read SHT3x (Temperature and Humidity)
        esp_err_t sht_err = sht3x_read_temp_humid(&temp, &humid, &raw_temp, &raw_humid);
        if (sht_err != ESP_OK) {
            ESP_LOGE(TAG, "SHT3x read failed (%s), using default compensation values", esp_err_to_name(sht_err));
            temp = 25.0f;
            humid = 50.0f;
            raw_temp = SGP41_DEFAULT_T_TICKS;
            raw_humid = SGP41_DEFAULT_RH_TICKS;
        }

        // Read SGP41 (VOC and NOx raw ticks) compensated with actual environment ticks
        esp_err_t sgp_err = sgp41_measure_raw(raw_temp, raw_humid, &sraw_voc, &sraw_nox);
        if (sgp_err != ESP_OK) {
            ESP_LOGE(TAG, "SGP41 read failed (%s)", esp_err_to_name(sgp_err));
        }

        // Read PMS7003 (PM2.5 values)
        esp_err_t pms_err = pms7003_read(&pm_data);
        if (pms_err == ESP_OK) {
            pm_valid = true;
        } else {
            // If read failed, keep using the last valid reading to avoid false drop-outs, 
            // but log a warning if we have never obtained a valid packet yet.
            if (!pm_valid) {
                memset(&pm_data, 0, sizeof(pm_data));
            }
        }

        // Apply humidity compensation for PM2.5 (PDF Equation 13)
        uint16_t pm25_final = pm_data.pm2_5_atm;
        if (pm_valid) {
            float rh_ratio = humid / 100.0f;
            float compensation = 1.0f + 0.25f * (rh_ratio * rh_ratio);
            pm25_final = (uint16_t)((float)pm_data.pm2_5_atm / compensation);
        }

        // Process data through Multi-Sensor Fusion Decision Engine
        float voc_drop = 0.0f;
        float voc_baseline = 0.0f;
        vape_status_t status = vape_logic_process(temp, humid, sraw_voc, sraw_nox, pm25_final, &voc_drop, &voc_baseline);

        // Display results
        ESP_LOGI(TAG, "--------------------------------------------------");
        ESP_LOGI(TAG, "T: %.2f C | RH: %.2f %% | PM2.5: %u ug/m3 %s", 
                 temp, humid, pm25_final, pm_valid ? "" : "[STALE]");
        ESP_LOGI(TAG, "VOC raw: %u | VOC base: %.1f | VOC drop: %.1f | NOx raw: %u", 
                 sraw_voc, voc_baseline, voc_drop, sraw_nox);

        // Alert formatting
        if (status == VAPE_STATUS_VAPE_DETECTED) {
            ESP_LOGE(TAG, ">>> !!! ALERT: %s !!! <<<", vape_status_to_str(status));
            buzzer_set(true); // Turn buzzer ON
        } else if (status != VAPE_STATUS_CLEAN) {
            ESP_LOGW(TAG, ">>> INFO: %s <<<", vape_status_to_str(status));
            buzzer_set(false); // Turn buzzer OFF
        } else {
            ESP_LOGI(TAG, "STATUS: %s", vape_status_to_str(status));
            buzzer_set(false); // Turn buzzer OFF
        }

        // Send data over BLE to connected web dashboard (JSON format)
        char ble_payload[128];
        snprintf(ble_payload, sizeof(ble_payload),
                 "{\"temp\":%.1f,\"humid\":%.1f,\"pm25\":%u,\"voc\":%u,\"status\":%d}",
                 temp, humid, pm25_final, sraw_voc, (int)status);
        ble_app_send_data(ble_payload);

        // Display metrics and status on OLED
        oled_clear();
        oled_draw_string(0, 0, "=== VAPE DETECTOR ===");
        
        char oled_trh[24];
        snprintf(oled_trh, sizeof(oled_trh), "T:%.1fC  RH:%.1f%%", temp, humid);
        oled_draw_string(0, 2, oled_trh);

        char oled_gas[24];
        snprintf(oled_gas, sizeof(oled_gas), "PM2.5:%d  VOC:%d", pm25_final, sraw_voc);
        oled_draw_string(0, 4, oled_gas);

        if (status == VAPE_STATUS_VAPE_DETECTED) {
            oled_draw_string(0, 6, "!!! VAPE ALERT !!!");
        } else if (status == VAPE_STATUS_STEAM_EXCLUDED) {
            oled_draw_string(0, 6, "STATE: STEAM EXCL");
        } else if (status == VAPE_STATUS_CHEMICAL_EXCLUDED) {
            oled_draw_string(0, 6, "STATE: CHEM EXCL");
        } else {
            oled_draw_string(0, 6, "STATE: CLEAN AIR");
        }
        oled_update();

        // Run cycle at exactly 1Hz
        vTaskDelayUntil(&start_tick, pdMS_TO_TICKS(1000));
    }
}
