#include "oled_display.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "oled";

/* ──────────── Font 5×7 ASCII 32–127 ──────────── */
static const uint8_t FONT6x8[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* 32 space */
    {0x00,0x00,0x5F,0x00,0x00}, /* 33 ! */
    {0x00,0x07,0x00,0x07,0x00}, /* 34 " */
    {0x14,0x7F,0x14,0x7F,0x14}, /* 35 # */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* 36 $ */
    {0x23,0x13,0x08,0x64,0x62}, /* 37 % */
    {0x36,0x49,0x55,0x22,0x50}, /* 38 & */
    {0x00,0x05,0x03,0x00,0x00}, /* 39 ' */
    {0x00,0x1C,0x22,0x41,0x00}, /* 40 ( */
    {0x00,0x41,0x22,0x1C,0x00}, /* 41 ) */
    {0x08,0x2A,0x1C,0x2A,0x08}, /* 42 * */
    {0x08,0x08,0x3E,0x08,0x08}, /* 43 + */
    {0x00,0x50,0x30,0x00,0x00}, /* 44 , */
    {0x08,0x08,0x08,0x08,0x08}, /* 45 - */
    {0x00,0x60,0x60,0x00,0x00}, /* 46 . */
    {0x20,0x10,0x08,0x04,0x02}, /* 47 / */
    {0x3E,0x51,0x49,0x45,0x3E}, /* 48 0 */
    {0x00,0x42,0x7F,0x40,0x00}, /* 49 1 */
    {0x42,0x61,0x51,0x49,0x46}, /* 50 2 */
    {0x21,0x41,0x45,0x4B,0x31}, /* 51 3 */
    {0x18,0x14,0x12,0x7F,0x10}, /* 52 4 */
    {0x27,0x45,0x45,0x45,0x39}, /* 53 5 */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 54 6 */
    {0x01,0x71,0x09,0x05,0x03}, /* 55 7 */
    {0x36,0x49,0x49,0x49,0x36}, /* 56 8 */
    {0x06,0x49,0x49,0x29,0x1E}, /* 57 9 */
    {0x00,0x36,0x36,0x00,0x00}, /* 58 : */
    {0x00,0x56,0x36,0x00,0x00}, /* 59 ; */
    {0x00,0x08,0x14,0x22,0x41}, /* 60 < */
    {0x14,0x14,0x14,0x14,0x14}, /* 61 = */
    {0x41,0x22,0x14,0x08,0x00}, /* 62 > */
    {0x02,0x01,0x51,0x09,0x06}, /* 63 ? */
    {0x32,0x49,0x79,0x41,0x3E}, /* 64 @ */
    {0x7E,0x11,0x11,0x11,0x7E}, /* 65 A */
    {0x7F,0x49,0x49,0x49,0x36}, /* 66 B */
    {0x3E,0x41,0x41,0x41,0x22}, /* 67 C */
    {0x7F,0x41,0x41,0x22,0x1C}, /* 68 D */
    {0x7F,0x49,0x49,0x49,0x41}, /* 69 E */
    {0x7F,0x09,0x09,0x09,0x01}, /* 70 F */
    {0x3E,0x41,0x49,0x49,0x7A}, /* 71 G */
    {0x7F,0x08,0x08,0x08,0x7F}, /* 72 H */
    {0x00,0x41,0x7F,0x41,0x00}, /* 73 I */
    {0x20,0x40,0x41,0x3F,0x01}, /* 74 J */
    {0x7F,0x08,0x14,0x22,0x41}, /* 75 K */
    {0x7F,0x40,0x40,0x40,0x40}, /* 76 L */
    {0x7F,0x02,0x04,0x02,0x7F}, /* 77 M */
    {0x7F,0x04,0x08,0x10,0x7F}, /* 78 N */
    {0x3E,0x41,0x41,0x41,0x3E}, /* 79 O */
    {0x7F,0x09,0x09,0x09,0x06}, /* 80 P */
    {0x3E,0x41,0x51,0x21,0x5E}, /* 81 Q */
    {0x7F,0x09,0x19,0x29,0x46}, /* 82 R */
    {0x46,0x49,0x49,0x49,0x31}, /* 83 S */
    {0x01,0x01,0x7F,0x01,0x01}, /* 84 T */
    {0x3F,0x40,0x40,0x40,0x3F}, /* 85 U */
    {0x1F,0x20,0x40,0x20,0x1F}, /* 86 V */
    {0x3F,0x40,0x38,0x40,0x3F}, /* 87 W */
    {0x63,0x14,0x08,0x14,0x63}, /* 88 X */
    {0x03,0x04,0x78,0x04,0x03}, /* 89 Y */
    {0x61,0x51,0x49,0x45,0x43}, /* 90 Z */
    {0x00,0x00,0x7F,0x41,0x41}, /* 91 [ */
    {0x02,0x04,0x08,0x10,0x20}, /* 92 \ */
    {0x41,0x41,0x7F,0x00,0x00}, /* 93 ] */
    {0x04,0x02,0x01,0x02,0x04}, /* 94 ^ */
    {0x40,0x40,0x40,0x40,0x40}, /* 95 _ */
    {0x00,0x01,0x02,0x04,0x00}, /* 96 ` */
    {0x20,0x54,0x54,0x54,0x78}, /* 97 a */
    {0x7F,0x48,0x44,0x44,0x38}, /* 98 b */
    {0x38,0x44,0x44,0x44,0x20}, /* 99 c */
    {0x38,0x44,0x44,0x48,0x7F}, /* 100 d */
    {0x38,0x54,0x54,0x54,0x18}, /* 101 e */
    {0x08,0x7E,0x09,0x01,0x02}, /* 102 f */
    {0x08,0x54,0x54,0x54,0x3C}, /* 103 g */
    {0x7F,0x08,0x04,0x04,0x78}, /* 104 h */
    {0x00,0x44,0x7D,0x40,0x00}, /* 105 i */
    {0x20,0x40,0x44,0x3D,0x00}, /* 106 j */
    {0x00,0x7F,0x10,0x28,0x44}, /* 107 k */
    {0x00,0x41,0x7F,0x40,0x00}, /* 108 l */
    {0x7C,0x04,0x18,0x04,0x78}, /* 109 m */
    {0x7C,0x08,0x04,0x04,0x78}, /* 110 n */
    {0x38,0x44,0x44,0x44,0x38}, /* 111 o */
    {0x7C,0x14,0x14,0x14,0x08}, /* 112 p */
    {0x08,0x14,0x14,0x18,0x7C}, /* 113 q */
    {0x7C,0x08,0x04,0x04,0x08}, /* 114 r */
    {0x48,0x54,0x54,0x54,0x20}, /* 115 s */
    {0x04,0x3F,0x44,0x40,0x20}, /* 116 t */
    {0x3C,0x40,0x40,0x40,0x3C}, /* 117 u */
    {0x1C,0x20,0x40,0x20,0x1C}, /* 118 v */
    {0x3C,0x40,0x30,0x40,0x3C}, /* 119 w */
    {0x44,0x28,0x10,0x28,0x44}, /* 120 x */
    {0x0C,0x50,0x50,0x50,0x3C}, /* 121 y */
    {0x44,0x64,0x54,0x4C,0x44}, /* 122 z */
    {0x00,0x08,0x36,0x41,0x00}, /* 123 { */
    {0x00,0x00,0x7F,0x00,0x00}, /* 124 | */
    {0x00,0x41,0x36,0x08,0x00}, /* 125 } */
    {0x08,0x08,0x2A,0x1C,0x08}, /* 126 ~ */
    {0x08,0x1C,0x2A,0x08,0x08}, /* 127 del */
};

/* ──────────── Frame buffer 128×64 / 8 = 1024 bytes ──────────── */
static uint8_t s_fb[OLED_WIDTH * (OLED_HEIGHT / 8)];

/* ──────────── Low-level I2C ──────────── */
static esp_err_t _i2c_write(const uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (OLED_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, (uint8_t *)data, len, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(OLED_I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static void _cmd(uint8_t c)
{
    uint8_t buf[2] = {0x00, c};
    _i2c_write(buf, 2);
}

/* ──────────── Init SSD1306 ──────────── */
esp_err_t oled_init(void)
{
    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = OLED_SDA_PIN,
        .scl_io_num       = OLED_SCL_PIN,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = OLED_I2C_FREQ,
    };
    ESP_ERROR_CHECK(i2c_param_config(OLED_I2C_PORT, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(OLED_I2C_PORT, I2C_MODE_MASTER, 0, 0, 0));

    static const uint8_t init_seq[] = {
        0xAE, 0xD5,0x80, 0xA8,0x3F, 0xD3,0x00, 0x40,
        0x8D,0x14, 0x20,0x00, 0xA1, 0xC8, 0xDA,0x12,
        0x81,0xCF, 0xD9,0xF1, 0xDB,0x40, 0xA4, 0xA6, 0xAF,
    };
    for (size_t i = 0; i < sizeof(init_seq); i++) _cmd(init_seq[i]);

    memset(s_fb, 0, sizeof(s_fb));
    ESP_LOGI(TAG, "SSD1306 init OK");
    return ESP_OK;
}

/* ──────────── Flush TOÀN BỘ frame buffer trong 1 I2C transaction ──────────── */
/*
 * Gốc gửi từng page riêng lẻ → nếu bị preempt giữa chừng thì frame bị vỡ.
 * Bản này gửi 1025 bytes (1 byte 0x40 + 1024 bytes data) trong 1 lần duy nhất.
 */
static void _flush(void)
{
    /* Set địa chỉ: toàn màn hình */
    _cmd(0x21); _cmd(0); _cmd(127);  /* col 0..127  */
    _cmd(0x22); _cmd(0); _cmd(7);    /* page 0..7   */

    /*
     * Gửi toàn bộ 1024 bytes trong 1 I2C transaction duy nhất.
     * Dùng i2c_cmd_link thay vì malloc 1025 bytes liên tục.
     */
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd,
        (OLED_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);

    uint8_t ctrl = 0x40;   /* Co=0, D/C=1 → data stream */
    i2c_master_write_byte(cmd, ctrl, true);
    i2c_master_write(cmd, s_fb, sizeof(s_fb), true);

    i2c_master_stop(cmd);
    i2c_master_cmd_begin(OLED_I2C_PORT, cmd, pdMS_TO_TICKS(200));
    i2c_cmd_link_delete(cmd);
}

/* ──────────── Vẽ ký tự ──────────── */
static void _draw_char(int x, int page, char c)
{
    if (c < 32 || c > 127) c = '?';
    const uint8_t *glyph = FONT6x8[c - 32];
    for (int col = 0; col < 5; col++) {
        if (x + col < OLED_WIDTH)
            s_fb[page * OLED_WIDTH + x + col] = glyph[col];
    }
    if (x + 5 < OLED_WIDTH)
        s_fb[page * OLED_WIDTH + x + 5] = 0x00;
}

static void _draw_str(int x, int page, const char *str)
{
    while (*str) {
        _draw_char(x, page, *str++);
        x += 6;
        if (x >= OLED_WIDTH) break;
    }
}

/* ──────────── Ký tự to 2× ──────────── */
static void _draw_char_big(int x, int page, char c)
{
    if (c < 32 || c > 127) c = '?';
    const uint8_t *glyph = FONT6x8[c - 32];
    for (int col = 0; col < 5; col++) {
        uint8_t byte = glyph[col];
        uint16_t stretched = 0;
        for (int b = 0; b < 8; b++) {
            if (byte & (1 << b)) stretched |= (3 << (b * 2));
        }
        uint8_t lo = stretched & 0xFF;
        uint8_t hi = (stretched >> 8) & 0xFF;
        int px = x + col * 2;
        if (px + 1 < OLED_WIDTH) {
            s_fb[page * OLED_WIDTH + px]     = lo;
            s_fb[page * OLED_WIDTH + px + 1] = lo;
            if (page + 1 < 8) {
                s_fb[(page+1) * OLED_WIDTH + px]     = hi;
                s_fb[(page+1) * OLED_WIDTH + px + 1] = hi;
            }
        }
    }
}

static void _draw_str_big(int x, int page, const char *str)
{
    while (*str) {
        _draw_char_big(x, page, *str++);
        x += 12;
        if (x >= OLED_WIDTH) break;
    }
}

/* ──────────── Icon REC ──────────── */
static void _draw_rec_icon(int x, int page)
{
    static const uint8_t circle[] = {0x3C,0x7E,0xFF,0xFF,0xFF,0xFF,0x7E,0x3C};
    for (int i = 0; i < 8 && x + i < OLED_WIDTH; i++)
        s_fb[page * OLED_WIDTH + x + i] = circle[i];
}

/* ──────────── Public API ──────────── */
void oled_show_ready(void)
{
    memset(s_fb, 0, sizeof(s_fb));
    _draw_str(16, 2, "INMP441 RECORDER");
    _draw_str(22, 4, "Nhan nut de ghi");
    _flush();
}

void oled_show_recording(uint32_t elapsed_sec, const char *filename)
{
    memset(s_fb, 0, sizeof(s_fb));
    _draw_rec_icon(4, 0);
    _draw_str(16, 0, "DANG GHI...");
    if (filename) _draw_str(0, 1, filename);
    char timer[16];
    snprintf(timer, sizeof(timer), "%02lu:%02lu",
             (unsigned long)(elapsed_sec / 60),
             (unsigned long)(elapsed_sec % 60));
    _draw_str_big(20, 3, timer);
    _flush();
}

void oled_show_stopped(uint32_t total_sec)
{
    memset(s_fb, 0, sizeof(s_fb));
    _draw_str(24, 1, "DUNG GHI");
    char msg[24];
    snprintf(msg, sizeof(msg), "Tong: %02lu:%02lu",
             (unsigned long)(total_sec / 60),
             (unsigned long)(total_sec % 60));
    _draw_str(12, 3, msg);
    _draw_str(4, 5, "Nhan nut de tiep");
    _flush();
}

void oled_show_uploading(void)
{
    memset(s_fb, 0, sizeof(s_fb));
    _draw_str(16, 2, "DANG GUI LEN...");
    _draw_str(28, 4, "Vui long doi");
    _flush();
}

void oled_show_result(const char *result)
{
    memset(s_fb, 0, sizeof(s_fb));
    _draw_str(28, 0, "KET QUA AI:");
    /* Tự xuống dòng nếu chuỗi dài */
    char line[22];
    int len = strlen(result);
    if (len <= 21) {
        _draw_str(0, 3, result);
    } else {
        strncpy(line, result, 21); line[21] = 0;
        _draw_str(0, 2, line);
        _draw_str(0, 4, result + 21);
    }
    _flush();
}