#include "vape_logic.h"
#include "esp_log.h"
#include <stddef.h>

static const char *TAG = "VAPE_LOGIC";

static float baseline_voc = 0.0f;
static bool is_initialized = false;
static bool is_vape_detected = false;

void vape_logic_init(void)
{
    baseline_voc = 0.0f;
    is_initialized = false;
    is_vape_detected = false;
    ESP_LOGI(TAG, "Vape detection logic initialized");
}

vape_status_t vape_logic_process(float temp, float humid, uint16_t sraw_voc, uint16_t sraw_nox, 
                                 uint16_t pm2_5, float *voc_drop, float *voc_baseline)
{
    // Initialize baseline on the first run
    if (!is_initialized) {
        baseline_voc = (float)sraw_voc;
        is_initialized = true;
        ESP_LOGI(TAG, "Initial VOC baseline established: %.1f ticks", baseline_voc);
    }

    // Calculate how much SRAW_VOC has dropped below the baseline
    // Note: SRAW_VOC ticks decrease as VOC concentration increases.
    float drop = baseline_voc - (float)sraw_voc;
    if (drop < 0.0f) {
        drop = 0.0f;
    }

    if (voc_drop != NULL) {
        *voc_drop = drop;
    }
    if (voc_baseline != NULL) {
        *voc_baseline = baseline_voc;
    }

    // =========================================================================
    // Thuật toán xác định STATUS CHỈ LẤY CỦA PM2.5 (loại bỏ VOC, nhiệt độ, độ ẩm)
    // Các thông số khác vẫn được đo, đọc và gửi đi bình thường.
    // =========================================================================
    if (!is_vape_detected) {
        if (pm2_5 >= PM25_VAPE_THRESHOLD) {
            is_vape_detected = true;
        }
    } else {
        if (pm2_5 < PM25_CLEAR_THRESHOLD) {
            is_vape_detected = false;
        }
    }

    vape_status_t status = is_vape_detected ? VAPE_STATUS_VAPE_DETECTED : VAPE_STATUS_CLEAN;

    // Update baseline only when there is no active vape event
    if (status == VAPE_STATUS_CLEAN) {
        if ((float)sraw_voc > baseline_voc) {
            // SRAW_VOC is higher than baseline (meaning cleaner air), adapt upward quickly
            baseline_voc = 0.05f * (float)sraw_voc + 0.95f * baseline_voc;
        } else {
            // SRAW_VOC is lower but stable, adapt downward very slowly
            baseline_voc = VOC_EMA_ALPHA * (float)sraw_voc + (1.0f - VOC_EMA_ALPHA) * baseline_voc;
        }
    }

    return status;
}

const char* vape_status_to_str(vape_status_t status)
{
    switch (status) {
        case VAPE_STATUS_CLEAN:
            return "CLEAN AIR";
        case VAPE_STATUS_VAPE_DETECTED:
            return "VAPE DETECTED";
        case VAPE_STATUS_STEAM_EXCLUDED:
            return "STEAM EXCLUDED (HUMID)";
        case VAPE_STATUS_CHEMICAL_EXCLUDED:
            return "CHEMICAL EXCLUDED (CLEANER)";
        default:
            return "UNKNOWN";
    }
}
