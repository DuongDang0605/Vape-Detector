#include "pms7003.h"
#include "esp_log.h"
#include "string.h"

static const char *TAG = "PMS7003";

esp_err_t pms7003_init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = PMS_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Configure UART parameters
    esp_err_t err = uart_param_config(PMS_UART_PORT, &uart_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART param config failed: %s", esp_err_to_name(err));
        return err;
    }

    // Set UART pins
    err = uart_set_pin(PMS_UART_PORT, PMS_TX_GPIO, PMS_RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART set pin failed: %s", esp_err_to_name(err));
        return err;
    }

    // Install UART driver with buffer
    err = uart_driver_install(PMS_UART_PORT, PMS_BUF_SIZE * 2, 0, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART driver install failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "UART connection initialized successfully (TX: GPIO%d, RX: GPIO%d, Baud: %d)", 
             PMS_TX_GPIO, PMS_RX_GPIO, PMS_UART_BAUD);
    return ESP_OK;
}

esp_err_t pms7003_read(pms7003_data_t *data)
{
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    static uint8_t buf[PMS_BUF_SIZE * 2];
    static size_t leftover_len = 0;

    size_t len = 0;
    uart_get_buffered_data_len(PMS_UART_PORT, &len);
    
    if (len == 0 && leftover_len < 32) {
        return ESP_ERR_NOT_FOUND;
    }

    // Append new bytes from UART to the leftover buffer
    size_t space_available = sizeof(buf) - leftover_len;
    size_t to_read = (len > space_available) ? space_available : len;

    int read_len = 0;
    if (to_read > 0) {
        read_len = uart_read_bytes(PMS_UART_PORT, buf + leftover_len, to_read, 0);
        if (read_len > 0) {
            leftover_len += read_len;
        }
    }

    if (leftover_len < 32) {
        return ESP_ERR_NOT_FOUND;
    }

    // Search backward to find the LATEST valid frame
    esp_err_t res = ESP_ERR_NOT_FOUND;
    int latest_frame_idx = -1;

    for (int i = leftover_len - 32; i >= 0; i--) {
        if (buf[i] == 0x42 && buf[i + 1] == 0x4D) {
            uint16_t frame_len = (buf[i + 2] << 8) | buf[i + 3];
            if (frame_len == 28) {
                uint16_t checksum = (buf[i + 30] << 8) | buf[i + 31];
                uint16_t calc_checksum = 0;
                for (int j = 0; j < 30; j++) {
                    calc_checksum += buf[i + j];
                }
                if (checksum == calc_checksum) {
                    // Valid frame found! Parse data:
                    data->pm1_0_std = (buf[i + 4] << 8) | buf[i + 5];
                    data->pm2_5_std = (buf[i + 6] << 8) | buf[i + 7];
                    data->pm10_std  = (buf[i + 8] << 8) | buf[i + 9];
                    data->pm1_0_atm = (buf[i + 10] << 8) | buf[i + 11];
                    data->pm2_5_atm = (buf[i + 12] << 8) | buf[i + 13];
                    data->pm10_atm  = (buf[i + 14] << 8) | buf[i + 15];
                    
                    res = ESP_OK;
                    latest_frame_idx = i;
                    break; // Stop searching because we found the latest one
                }
            }
        }
    }

    if (latest_frame_idx >= 0) {
        // A frame was found and consumed. 
        // Discard all bytes up to the end of this frame to clear the backlog.
        // Keep any remaining bytes that arrived AFTER this frame for the next read.
        size_t consumed = latest_frame_idx + 32;
        size_t remaining = leftover_len - consumed;
        if (remaining > 0) {
            memmove(buf, buf + consumed, remaining);
            leftover_len = remaining;
        } else {
            leftover_len = 0;
        }
    } else {
        // No valid frame found in the entire buffer.
        // If buffer is getting full, retain only the last 31 bytes (which might be the head of a new frame).
        if (leftover_len > sizeof(buf) - 64) {
            memmove(buf, buf + leftover_len - 31, 31);
            leftover_len = 31;
        }
    }

    // Flush any remaining unread bytes in UART driver ring buffer if we couldn't read them all (Catch up to real-time)
    if (len > space_available) {
        uart_flush(PMS_UART_PORT);
    }

    return res;
}
