#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Cấu hình SD Card SPI ========== */
#define SD_PIN_CS    10
#define SD_PIN_MOSI  11
#define SD_PIN_SCK   13
#define SD_PIN_MISO  12
#define SD_MOUNT     "/sdcard"

/* Số file tối đa lưu trên thẻ — khi đầy sẽ ghi đè file cũ nhất */
#define SD_MAX_FILES  999

/**
 * @brief Khởi tạo SPI và mount thẻ SD
 * @return ESP_OK nếu thành công
 */
esp_err_t sd_wav_init(void);

/**
 * @brief Unmount thẻ SD và giải phóng SPI
 */
void sd_wav_deinit(void);

/**
 * @brief Mở file WAV mới để ghi (tự đặt tên rec_001.wav, rec_002.wav...)
 *        Nếu đã có SD_MAX_FILES file, xóa file cũ nhất trước.
 * @param sample_rate   Tần số lấy mẫu (Hz)
 * @param channels      Số kênh (1 = mono)
 * @return ESP_OK nếu mở thành công
 */
esp_err_t sd_wav_open(uint32_t sample_rate, uint8_t channels);

/**
 * @brief Ghi thêm PCM int16 vào file đang mở
 * @param samples  Con trỏ buffer int16
 * @param n_samp   Số samples cần ghi
 * @return ESP_OK nếu thành công
 */
esp_err_t sd_wav_write(const int16_t *samples, size_t n_samp);

/**
 * @brief Đóng file WAV, cập nhật header với kích thước thực tế
 * @return ESP_OK nếu thành công
 */
esp_err_t sd_wav_close(void);

/**
 * @brief Kiểm tra thẻ SD đã mount chưa
 */
bool sd_wav_is_mounted(void);
void sd_wav_get_last_path(char *buf, size_t size);
#ifdef __cplusplus
}
#endif