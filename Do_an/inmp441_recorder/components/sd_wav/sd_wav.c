#include "sd_wav.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char *TAG = "sd_wav";

/* ──────────── WAV header (44 bytes chuẩn PCM) ──────────── */
typedef struct __attribute__((packed)) {
    /* RIFF chunk */
    char     riff_id[4];        /* "RIFF" */
    uint32_t riff_size;         /* file size - 8 */
    char     wave_id[4];        /* "WAVE" */
    /* fmt chunk */
    char     fmt_id[4];         /* "fmt " */
    uint32_t fmt_size;          /* 16 */
    uint16_t audio_format;      /* 1 = PCM */
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;         /* sample_rate * channels * bits/8 */
    uint16_t block_align;       /* channels * bits/8 */
    uint16_t bits_per_sample;   /* 16 */
    /* data chunk */
    char     data_id[4];        /* "data" */
    uint32_t data_size;         /* số byte PCM */
} wav_header_t;

/* ──────────── State nội bộ ──────────── */
static sdmmc_card_t *s_card    = NULL;
static FILE         *s_file    = NULL;
static uint32_t      s_data_bytes  = 0;
static uint32_t      s_sample_rate = 0;
static uint8_t       s_channels    = 0;
static bool          s_mounted = false;
static char          s_filepath[64];

/* ──────────── Helpers ──────────── */

static void _make_wav_header(wav_header_t *h,
                              uint32_t sample_rate,
                              uint8_t  channels,
                              uint32_t data_bytes)
{
    memcpy(h->riff_id,  "RIFF", 4);
    h->riff_size      = data_bytes + sizeof(wav_header_t) - 8;
    memcpy(h->wave_id,  "WAVE", 4);
    memcpy(h->fmt_id,   "fmt ", 4);
    h->fmt_size        = 16;
    h->audio_format    = 1;
    h->num_channels    = channels;
    h->sample_rate     = sample_rate;
    h->bits_per_sample = 16;
    h->byte_rate       = sample_rate * channels * 2;
    h->block_align     = channels * 2;
    memcpy(h->data_id,  "data", 4);
    h->data_size       = data_bytes;
}

/**
 * Tìm số thứ tự tiếp theo cho file rec_XXX.wav.
 * Nếu đã có SD_MAX_FILES file → xóa file có số nhỏ nhất (cũ nhất).
 * Trả về số thứ tự mới (1..SD_MAX_FILES).
 */
static int _next_index(void)
{
    int  present[SD_MAX_FILES + 1];  /* present[i] = 1 nếu rec_i tồn tại */
    int  count = 0;
    int  min_idx = SD_MAX_FILES + 1;
    memset(present, 0, sizeof(present));

    for (int i = 1; i <= SD_MAX_FILES; i++) {
        char path[64];
        snprintf(path, sizeof(path), "%s/rec_%03d.wav", SD_MOUNT, i);
        FILE *f = fopen(path, "r");
        if (f) {
            fclose(f);
            present[i] = 1;
            count++;
            if (i < min_idx) min_idx = i;
        }
    }

    if (count >= SD_MAX_FILES) {
        /* Xóa file cũ nhất */
        char del_path[64];
        snprintf(del_path, sizeof(del_path), "%s/rec_%03d.wav", SD_MOUNT, min_idx);
        remove(del_path);
        ESP_LOGW(TAG, "The SD day — da xoa file cu nhat: %s", del_path);
        present[min_idx] = 0;
    }

    /* Tìm số nhỏ nhất chưa có */
    for (int i = 1; i <= SD_MAX_FILES; i++) {
        if (!present[i]) return i;
    }
    return 1; /* fallback */
}

/* ──────────── Public API ──────────── */

esp_err_t sd_wav_init(void)
{
    if (s_mounted) return ESP_OK;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = SD_PIN_MOSI,
        .miso_io_num     = SD_PIN_MISO,
        .sclk_io_num     = SD_PIN_SCK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 4096,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(SPI2_HOST, &bus_cfg, SDSPI_DEFAULT_DMA),
                        TAG, "SPI bus init that bai");

    sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot.gpio_cs = SD_PIN_CS;
    slot.host_id = SPI2_HOST;
    slot.gpio_cd = SDSPI_SLOT_NO_CD;

    sdmmc_host_t host    = SDSPI_HOST_DEFAULT();
    host.flags           = SDMMC_HOST_FLAG_SPI | SDMMC_HOST_FLAG_DEINIT_ARG;
    host.max_freq_khz    = 20000;   /* 20 MHz — tăng lên sau khi init thành công */

    esp_vfs_fat_sdmmc_mount_config_t mnt = {
        .format_if_mount_failed = false,
        .max_files              = 5,
        .allocation_unit_size   = 16 * 1024,
    };

    esp_err_t ret = esp_vfs_fat_sdspi_mount(SD_MOUNT, &host, &slot, &mnt, &s_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Mount SD that bai: %s", esp_err_to_name(ret));
        spi_bus_free(SPI2_HOST);
        return ret;
    }

    s_mounted = true;
    ESP_LOGI(TAG, "SD mount OK — %.0f MB",
             (double)s_card->csd.capacity * s_card->csd.sector_size / (1024.0 * 1024.0));
    return ESP_OK;
}

/* ──────────────────────────────────── */

void sd_wav_deinit(void)
{
    if (s_file) sd_wav_close();
    if (!s_mounted) return;
    esp_vfs_fat_sdcard_unmount(SD_MOUNT, s_card);
    spi_bus_free(SPI2_HOST);
    s_mounted = false;
    s_card    = NULL;
    ESP_LOGI(TAG, "SD unmount OK");
}

/* ──────────────────────────────────── */

esp_err_t sd_wav_open(uint32_t sample_rate, uint8_t channels)
{
    if (!s_mounted) {
        ESP_LOGE(TAG, "SD chua mount");
        return ESP_ERR_INVALID_STATE;
    }
    if (s_file) {
        ESP_LOGW(TAG, "File dang mo, dong truoc");
        sd_wav_close();
    }

    int idx = _next_index();
    snprintf(s_filepath, sizeof(s_filepath), "%s/rec_%03d.wav", SD_MOUNT, idx);

    s_file = fopen(s_filepath, "wb");
    if (!s_file) {
        ESP_LOGE(TAG, "Khong mo duoc file: %s", s_filepath);
        return ESP_FAIL;
    }

    /* Ghi header tạm với data_size = 0, sẽ cập nhật khi đóng file */
    wav_header_t hdr;
    _make_wav_header(&hdr, sample_rate, channels, 0);
    fwrite(&hdr, sizeof(hdr), 1, s_file);

    s_data_bytes  = 0;
    s_sample_rate = sample_rate;
    s_channels    = channels;
    ESP_LOGI(TAG, "Bat dau ghi: %s (%lu Hz, %d ch)", s_filepath, sample_rate, channels);
    return ESP_OK;
}

/* ──────────────────────────────────── */

esp_err_t sd_wav_write(const int16_t *samples, size_t n_samp)
{
    if (!s_file) return ESP_ERR_INVALID_STATE;

    size_t written = fwrite(samples, sizeof(int16_t), n_samp, s_file);
    if (written != n_samp) {
        ESP_LOGE(TAG, "Ghi that bai: %zu/%zu samples", written, n_samp);
        return ESP_FAIL;
    }

    s_data_bytes += (uint32_t)(written * sizeof(int16_t));
    return ESP_OK;
}

/* ──────────────────────────────────── */

esp_err_t sd_wav_close(void)
{
    if (!s_file) return ESP_ERR_INVALID_STATE;

    /* Ghi lại header đúng với data_size thực tế */
    wav_header_t hdr;
    _make_wav_header(&hdr, s_sample_rate, s_channels, s_data_bytes);

    rewind(s_file);
    fwrite(&hdr, sizeof(hdr), 1, s_file);
    fclose(s_file);
    s_file = NULL;

    float dur = (float)s_data_bytes / (s_sample_rate * s_channels * 2);
    ESP_LOGI(TAG, "Luu xong: %s — %.1f giay, %lu KB",
             s_filepath, dur, (s_data_bytes + sizeof(hdr)) / 1024);
    return ESP_OK;
}

/* ──────────────────────────────────── */

bool sd_wav_is_mounted(void)
{
    return s_mounted;
}
void sd_wav_get_last_path(char *buf, size_t size)
{
    strncpy(buf, s_filepath, size - 1);
    buf[size - 1] = '\0';
}