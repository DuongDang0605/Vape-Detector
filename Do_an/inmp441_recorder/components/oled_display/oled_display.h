#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Cấu hình I2C / OLED ========== */
#define OLED_SDA_PIN     22
#define OLED_SCL_PIN     21
#define OLED_I2C_PORT    I2C_NUM_0
#define OLED_I2C_FREQ    400000
#define OLED_I2C_ADDR    0x3C
#define OLED_WIDTH       128
#define OLED_HEIGHT      64

/** Khởi tạo I2C và SSD1306 */
esp_err_t oled_init(void);

/** Màn hình chờ */
void oled_show_ready(void);

/** Đang ghi — hiển thị timer MM:SS và tên file */
void oled_show_recording(uint32_t elapsed_sec, const char *filename);

/** Dừng ghi — hiển thị tổng thời gian */
void oled_show_stopped(uint32_t total_sec);

/** Đang upload lên server */
void oled_show_uploading(void);

/** Hiển thị kết quả AI trả về */
void oled_show_result(const char *result);

#ifdef __cplusplus
}
#endif