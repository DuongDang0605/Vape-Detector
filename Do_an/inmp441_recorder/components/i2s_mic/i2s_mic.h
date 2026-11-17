#pragma once

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Cấu hình INMP441 ========== */
#define MIC_WS_PIN       5
#define MIC_SD_PIN       6
#define MIC_SCK_PIN      7

#define MIC_SAMPLE_RATE  4000
#define MIC_DMA_BUF_CNT  8
#define MIC_DMA_BUF_LEN  1023

/**
 * @brief Khởi tạo I2S cho INMP441
 * @return ESP_OK nếu thành công
 */
esp_err_t i2s_mic_init(void);

/**
 * @brief Deinit và giải phóng tài nguyên I2S
 */
void i2s_mic_deinit(void);

/**
 * @brief Đọc samples từ mic, chuyển 32-bit MSB → int16
 * @param buf     Buffer int16 nhận data
 * @param n_samp  Số samples muốn đọc
 * @param got     Số samples thực sự đọc được
 * @param timeout_ms Timeout tính bằng ms
 * @return ESP_OK, ESP_ERR_TIMEOUT, hoặc lỗi khác
 */
esp_err_t i2s_mic_read(int16_t *buf, size_t n_samp,
                        size_t *got, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif