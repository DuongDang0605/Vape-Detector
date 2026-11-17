#include "sgp41.h"
#include "i2c_bus.h"
#include "sht3x.h" // For sensirion_crc8 function
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "SGP41";

/**
 * @brief Helper to format and send commands with parameters (RH and T ticks)
 */
static esp_err_t send_cmd_with_params(uint16_t cmd, uint16_t raw_humid, uint16_t raw_temp)
{
    uint8_t payload[8];
    payload[0] = (uint8_t)(cmd >> 8);
    payload[1] = (uint8_t)(cmd & 0xFF);
    
    payload[2] = (uint8_t)(raw_humid >> 8);
    payload[3] = (uint8_t)(raw_humid & 0xFF);
    payload[4] = sensirion_crc8(payload + 2, 2);
    
    payload[5] = (uint8_t)(raw_temp >> 8);
    payload[6] = (uint8_t)(raw_temp & 0xFF);
    payload[7] = sensirion_crc8(payload + 5, 2);

    return i2c_bus_write_bytes(SGP41_I2C_ADDR, payload, sizeof(payload));
}

esp_err_t sgp41_init(void)
{
    // Try to run a measurement with default ticks to see if the sensor is present and responsive
    uint16_t voc = 0, nox = 0;
    esp_err_t err = sgp41_measure_raw(SGP41_DEFAULT_T_TICKS, SGP41_DEFAULT_RH_TICKS, &voc, &nox);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "SGP41 detected and verified successfully (VOC raw: %d, NOx raw: %d)", voc, nox);
    } else {
        ESP_LOGE(TAG, "Failed to communicate with SGP41 sensor: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t sgp41_execute_conditioning(uint16_t raw_temp, uint16_t raw_humid, uint16_t *sraw_voc)
{
    if (sraw_voc == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = send_cmd_with_params(SGP41_CMD_EXEC_COND, raw_humid, raw_temp);
    if (err != ESP_OK) {
        return err;
    }

    // Wait for the conditioning step to complete (max 50ms)
    vTaskDelay(pdMS_TO_TICKS(50));

    // Read 3 bytes (SRAW_VOC word + CRC)
    uint8_t buf[3];
    err = i2c_bus_read_bytes(SGP41_I2C_ADDR, buf, sizeof(buf));
    if (err != ESP_OK) {
        return err;
    }

    // Check CRC
    if (sensirion_crc8(buf, 2) != buf[2]) {
        ESP_LOGE(TAG, "Conditioning SRAW_VOC CRC check failed!");
        return ESP_ERR_INVALID_CRC;
    }

    *sraw_voc = (buf[0] << 8) | buf[1];
    return ESP_OK;
}

esp_err_t sgp41_measure_raw(uint16_t raw_temp, uint16_t raw_humid, uint16_t *sraw_voc, uint16_t *sraw_nox)
{
    if (sraw_voc == NULL || sraw_nox == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = send_cmd_with_params(SGP41_CMD_MEAS_RAW, raw_humid, raw_temp);
    if (err != ESP_OK) {
        return err;
    }

    // Wait for measurement to complete (max 50ms)
    vTaskDelay(pdMS_TO_TICKS(50));

    // Read 6 bytes (SRAW_VOC word + CRC, SRAW_NOX word + CRC)
    uint8_t buf[6];
    err = i2c_bus_read_bytes(SGP41_I2C_ADDR, buf, sizeof(buf));
    if (err != ESP_OK) {
        return err;
    }

    // Check VOC CRC
    if (sensirion_crc8(buf, 2) != buf[2]) {
        ESP_LOGE(TAG, "Measurement SRAW_VOC CRC check failed!");
        return ESP_ERR_INVALID_CRC;
    }

    // Check NOx CRC
    if (sensirion_crc8(buf + 3, 2) != buf[5]) {
        ESP_LOGE(TAG, "Measurement SRAW_NOX CRC check failed!");
        return ESP_ERR_INVALID_CRC;
    }

    *sraw_voc = (buf[0] << 8) | buf[1];
    *sraw_nox = (buf[3] << 8) | buf[4];

    return ESP_OK;
}
