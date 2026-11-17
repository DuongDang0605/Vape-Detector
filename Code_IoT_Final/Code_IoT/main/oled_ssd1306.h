#ifndef OLED_SSD1306_H
#define OLED_SSD1306_H

#include <stdint.h>
#include "esp_err.h"

#define OLED_I2C_ADDR                  0x3C    /*!< SSD1306 default I2C address */

/**
 * @brief Initialize the SSD1306 OLED display
 * 
 * @return esp_err_t ESP_OK on success, or error code
 */
esp_err_t oled_init(void);

/**
 * @brief Clear the display buffer (set all pixels to off/black)
 */
void oled_clear(void);

/**
 * @brief Send the current local display buffer to the screen
 * 
 * @return esp_err_t ESP_OK on success, or error code
 */
esp_err_t oled_update(void);

/**
 * @brief Draw a single character in the framebuffer
 * 
 * @param x Pixel column index (0-127)
 * @param page Page row index (0-7, where each page is 8 pixels high)
 * @param c The ASCII character to render
 */
void oled_draw_char(uint8_t x, uint8_t page, char c);

/**
 * @brief Draw a string of characters in the framebuffer
 * 
 * @param x Pixel column index (0-127)
 * @param page Page row index (0-7, where each page is 8 pixels high)
 * @param str Null-terminated string to render
 */
void oled_draw_string(uint8_t x, uint8_t page, const char *str);

#endif // OLED_SSD1306_H
