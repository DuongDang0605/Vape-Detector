#include "i2s_mic.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "i2s_mic";

/* Buffer 32-bit nội bộ để đọc raw từ I2S */
#define RAW_BUF_SAMPLES  (MIC_DMA_BUF_LEN)
static int32_t s_raw_buf[RAW_BUF_SAMPLES];

static i2s_chan_handle_t s_rx = NULL;

/* ──────────────────────────────────────── */

esp_err_t i2s_mic_init(void)
{
    if (s_rx != NULL) {
        ESP_LOGW(TAG, "Da init roi, bo qua");
        return ESP_OK;
    }

    i2s_chan_config_t chan_cfg = {
        .id            = I2S_NUM_0,
        .role          = I2S_ROLE_MASTER,
        .dma_desc_num  = MIC_DMA_BUF_CNT,
        .dma_frame_num = MIC_DMA_BUF_LEN,
        .auto_clear    = false,
    };
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, NULL, &s_rx),
                        TAG, "i2s_new_channel that bai");

    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(MIC_SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(
                        I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk         = I2S_GPIO_UNUSED,
            .bclk         = MIC_SCK_PIN,
            .ws           = MIC_WS_PIN,
            .dout         = I2S_GPIO_UNUSED,
            .din          = MIC_SD_PIN,
            .invert_flags = { false, false, false },
        },
    };
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;

    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(s_rx, &std_cfg),
                        TAG, "init std mode that bai");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(s_rx),
                        TAG, "enable channel that bai");

    ESP_LOGI(TAG, "INMP441 init OK (%d Hz)", MIC_SAMPLE_RATE);
    return ESP_OK;
}

/* ──────────────────────────────────────── */

void i2s_mic_deinit(void)
{
    if (s_rx == NULL) return;
    i2s_channel_disable(s_rx);
    i2s_del_channel(s_rx);
    s_rx = NULL;
    ESP_LOGI(TAG, "Deinit OK");
}

/* ──────────────────────────────────────── */

esp_err_t i2s_mic_read(int16_t *buf, size_t n_samp,
                        size_t *got, uint32_t timeout_ms)
{
    *got = 0;
    size_t remaining = n_samp;

    while (remaining > 0) {
        size_t batch = remaining < RAW_BUF_SAMPLES ? remaining : RAW_BUF_SAMPLES;
        size_t bytes_read = 0;

        esp_err_t ret = i2s_channel_read(s_rx,
                                         s_raw_buf,
                                         batch * sizeof(int32_t),
                                         &bytes_read,
                                         pdMS_TO_TICKS(timeout_ms));

        if (ret == ESP_ERR_TIMEOUT) return ESP_ERR_TIMEOUT;
        if (ret != ESP_OK)          return ret;

        size_t read_samp = bytes_read / sizeof(int32_t);

        /* Chuyển 32-bit MSB → int16 (INMP441 data ở bit 23..8) */
        for (size_t i = 0; i < read_samp; i++) {
            buf[*got + i] = (int16_t)(s_raw_buf[i] >> 8);
        }

        *got      += read_samp;
        remaining -= read_samp;
    }

    return ESP_OK;
}