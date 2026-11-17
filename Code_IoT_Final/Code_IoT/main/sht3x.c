#include "sht3x.h"
#include "i2c_bus.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "SHT3X";

uint8_t sensirion_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

esp_err_t sht3x_init(void)
{
    // SHT3x does not require special initialization over I2C,
    // but we check if it is responsive by trying to read its status or taking a quick measurement.
    float temp, humid;
    uint16_t rt, rh;
    esp_err_t err = sht3x_read_temp_humid(&temp, &humid, &rt, &rh);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "SHT3x detected and communication verified successfully (T: %.2fC, RH: %.2f%%)", temp, humid);
    } else {
        ESP_LOGE(TAG, "Failed to communicate with SHT3x sensor: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t sht3x_read_temp_humid(float *temp, float *humid, uint16_t *raw_temp, uint16_t *raw_humid)
{
    if (temp == NULL || humid == NULL || raw_temp == NULL || raw_humid == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t cmd[2] = {
        (uint8_t)(SHT3X_CMD_MEAS_NO_STR >> 8),
        (uint8_t)(SHT3X_CMD_MEAS_NO_STR & 0xFF)
    };

    // Send single shot measurement command
    esp_err_t err = i2c_bus_write_bytes(SHT3X_I2C_ADDR, cmd, sizeof(cmd));
    if (err != ESP_OK) {
        return err;
    }

    // Wait for the measurement to complete (high repeatability max 15ms)
    vTaskDelay(pdMS_TO_TICKS(20));

    // Read 6 bytes of data
    uint8_t buf[6];
    err = i2c_bus_read_bytes(SHT3X_I2C_ADDR, buf, sizeof(buf));
    if (err != ESP_OK) {
        return err;
    }

    // Verify CRC for Temperature
    if (sensirion_crc8(buf, 2) != buf[2]) {
        ESP_LOGE(TAG, "Temperature CRC check failed!");
        return ESP_ERR_INVALID_CRC;
    }

    // Verify CRC for Humidity
    if (sensirion_crc8(buf + 3, 2) != buf[5]) {
        ESP_LOGE(TAG, "Humidity CRC check failed!");
        return ESP_ERR_INVALID_CRC;
    }

    // Parse values
    uint16_t raw_t_val = (buf[0] << 8) | buf[1];
    uint16_t raw_rh_val = (buf[3] << 8) | buf[4];

    *raw_temp = raw_t_val;
    *raw_humid = raw_rh_val;

    // Convert raw values to physics quantities
    *temp = -45.0f + 175.0f * (float)raw_t_val / 65535.0f;
    *humid = 100.0f * (float)raw_rh_val / 65535.0f;

    return ESP_OK;
}
