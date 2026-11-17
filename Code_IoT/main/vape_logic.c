#include "vape_logic.h"
#include "esp_log.h"

static const char *TAG = "VAPE_LOGIC";

static float baseline_voc = 0.0f;
static float baseline_pm25 = 0.0f;
static bool is_initialized = false;

void vape_logic_init(void)
{
    baseline_voc = 0.0f;
    is_initialized = false;
    ESP_LOGI(TAG, "Vape detection logic initialized");
}

void vape_logic_set_pm25_baseline(float baseline)
{
    baseline_pm25 = baseline;
    ESP_LOGI(TAG, "PM2.5 dynamic baseline set to: %.1f", baseline_pm25);
}

vape_status_t vape_logic_process(float temp, float humid, uint16_t sraw_voc, uint16_t sraw_nox, 
                                 uint16_t pm2_5, float *voc_drop, float *voc_baseline)
{
    // Initialize VOC baseline on the first run (PM2.5 baseline is set by main.c)
    if (!is_initialized) {
        baseline_voc = (float)sraw_voc;
        if (baseline_pm25 == 0.0f) baseline_pm25 = (float)pm2_5; // Fallback
        is_initialized = true;
        ESP_LOGI(TAG, "Initial VOC baseline established: %.1f ticks", baseline_voc);
    }

    // Calculate how much SRAW_VOC has dropped below the baseline
    // Note: SRAW_VOC ticks decrease as VOC concentration increases.
    float drop = baseline_voc - (float)sraw_voc;
    if (drop < 0.0f) {
        drop = 0.0f;
    }

    *voc_drop = drop;
    *voc_baseline = baseline_voc;

    // Calculate how much PM2.5 has risen above the dynamic baseline
    float pm25_rise = (float)pm2_5 - baseline_pm25;
    if (pm25_rise < 0.0f) {
        pm25_rise = 0.0f;
    }

    vape_status_t status = VAPE_STATUS_CLEAN;

    // Apply Multi-Sensor Fusion Decision Rules
    if (drop > VOC_DROP_CHEMICAL_THRESHOLD && pm2_5 < PM25_CLEAN_THRESHOLD) {
        // Rule 1: High VOC increase but low PM2.5 -> Cleaning Agent (Clo, Javen)
        status = VAPE_STATUS_CHEMICAL_EXCLUDED;
    } 
    else if (humid > HUMIDITY_STEAM_THRESHOLD && drop < VOC_DROP_STABLE_THRESHOLD) {
        // Rule 2: High humidity but stable VOC -> Humid steam from shower/tap (Ignore PM2.5 rise if any)
        status = VAPE_STATUS_STEAM_EXCLUDED;
    } 
    else if (pm25_rise > PM25_RISE_VAPE_THRESHOLD && drop > VOC_DROP_SPIKE_THRESHOLD) {
        // Rule 3: High PM2.5 rise and substantial VOC increase -> Vape smoke detected
        status = VAPE_STATUS_VAPE_DETECTED;
    } 
    else {
        // Default: Clean or no clear signature met
        status = VAPE_STATUS_CLEAN;
    }

    // Update baseline only when there is no active vape or chemical event 
    // to prevent the baseline from adapting to temporary pollution spikes.
    if (status == VAPE_STATUS_CLEAN || status == VAPE_STATUS_STEAM_EXCLUDED) {
        // Update VOC baseline
        if ((float)sraw_voc > baseline_voc) {
            // SRAW_VOC is higher than baseline (meaning cleaner air), adapt upward quickly
            baseline_voc = 0.05f * (float)sraw_voc + 0.95f * baseline_voc;
        } else {
            // SRAW_VOC is lower but stable, adapt downward very slowly
            baseline_voc = VOC_EMA_ALPHA * (float)sraw_voc + (1.0f - VOC_EMA_ALPHA) * baseline_voc;
        }

        // Update PM2.5 dynamic baseline to adapt to natural environmental drift
        if ((float)pm2_5 < baseline_pm25) {
            // Air is cleaner, adapt downward quickly
            baseline_pm25 = 0.05f * (float)pm2_5 + 0.95f * baseline_pm25;
        } else {
            // Air is slightly dirtier, adapt upward very slowly
            baseline_pm25 = 0.001f * (float)pm2_5 + 0.999f * baseline_pm25;
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
