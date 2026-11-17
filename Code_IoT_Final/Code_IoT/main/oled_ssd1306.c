#include "oled_ssd1306.h"
#include "i2c_bus.h"
#include "esp_log.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "OLED";

// 1025-byte buffer: first byte is the I2C control byte (0x40 for data), followed by 1024 bytes of GDDRAM
static uint8_t oled_payload[1025];
#define oled_buffer (&oled_payload[1])

// Font imported from glcdfont.c
extern const unsigned char font[];

/**
 * @brief Helper to send a stream of commands to SSD1306
 */
static esp_err_t send_commands(const uint8_t *cmds, size_t size)
{
    uint8_t temp_buf[64];
    if (size > 63) {
        return ESP_ERR_INVALID_SIZE;
    }
    temp_buf[0] = 0x00; // Control byte: all following bytes are command bytes
    memcpy(temp_buf + 1, cmds, size);
    return i2c_bus_write_bytes(OLED_I2C_ADDR, temp_buf, size + 1);
}

esp_err_t oled_init(void)
{
    // Initialize the control byte in the payload
    oled_payload[0] = 0x40; 
    
    // Clear screen buffer initially
    oled_clear();

    // Standard initialization command sequence for SSD1306 128x64 display
    static const uint8_t init_cmds[] = {
        0xAE,       // 1. Display OFF
        0xD5, 0x80, // 2. Set Display Clock Divide Ratio/Oscillator Frequency (default 0x80)
        0xA8, 0x3F, // 3. Set Multiplex Ratio (64MUX for 128x64)
        0xD3, 0x00, // 4. Set Display Offset (no offset)
        0x40,       // 5. Set Display Start Line (0)
        0x8D, 0x14, // 6. Enable Internal Charge Pump (0x14)
        0x20, 0x00, // 7. Set Memory Addressing Mode to Horizontal Mode (0x00)
        0xA1,       // 8. Set Segment Re-map (Column 127 is mapped to SEG0) - flips horizontally
        0xC8,       // 9. Set COM Output Scan Direction (re-mapped mode) - flips vertically
        0xDA, 0x12, // 10. Set COM Pins Hardware Configuration (alternative)
        0x81, 0xCF, // 11. Set Contrast Control (0xCF)
        0xD9, 0xF1, // 12. Set Pre-charge Period (0xF1)
        0xDB, 0x40, // 13. Set VCOMH Deselect Level
        0xA4,       // 14. Entire Display ON (Resume to RAM content)
        0xA6,       // 15. Set Normal Display (non-inverted)
        0xAF        // 16. Display ON
    };

    esp_err_t err = send_commands(init_cmds, sizeof(init_cmds));
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "SSD1306 OLED initialized successfully on shared bus (Addr: 0x%X)", OLED_I2C_ADDR);
        // Clear screen to remove potential noise
        oled_update();
    } else {
        ESP_LOGE(TAG, "Failed to initialize SSD1306 OLED: %s", esp_err_to_name(err));
    }
    return err;
}

void oled_clear(void)
{
    memset(oled_buffer, 0, 1024);
}

esp_err_t oled_update(void)
{
    // Commands to reset column and page address pointers to cover the entire screen in horizontal mode
    static const uint8_t range_cmds[] = {
        0x21, 0, 127, // Column address: 0 to 127
        0x22, 0, 7    // Page address: 0 to 7
    };

    esp_err_t err = send_commands(range_cmds, sizeof(range_cmds));
    if (err != ESP_OK) {
        return err;
    }

    // Write the entire 1025-byte payload (control byte + 1024 bytes buffer)
    return i2c_bus_write_bytes(OLED_I2C_ADDR, oled_payload, sizeof(oled_payload));
}

void oled_draw_char(uint8_t x, uint8_t page, char c)
{
    if (page >= 8 || x >= 128) {
        return;
    }

    // glcdfont.c has characters mapped from ASCII 0 to 255.
    // Each character is 5 bytes wide (5 vertical columns).
    // We add 1 blank pixel column as space between characters (total character width = 6 pixels).
    uint16_t font_offset = (uint16_t)c * 5;
    
    for (int i = 0; i < 5; i++) {
        if (x + i >= 128) {
            break;
        }
        oled_buffer[page * 128 + x + i] = font[font_offset + i];
    }
    
    // Add 1-pixel separator column
    if (x + 5 < 128) {
        oled_buffer[page * 128 + x + 5] = 0x00;
    }
}

void oled_draw_string(uint8_t x, uint8_t page, const char *str)
{
    if (str == NULL) {
        return;
    }
    
    while (*str) {
        oled_draw_char(x, page, *str);
        x += 6; // Shift column index by 6 pixels (5 font width + 1 spacing)
        if (x >= 128) {
            break; // Exceeds screen boundary
        }
        str++;
    }
}
