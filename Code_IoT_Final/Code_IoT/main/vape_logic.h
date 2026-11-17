#ifndef VAPE_LOGIC_H
#define VAPE_LOGIC_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Detection status result from sensor readings
 */
typedef enum {
    VAPE_STATUS_CLEAN = 0,         /*!< Normal clean air */
    VAPE_STATUS_VAPE_DETECTED,    /*!< Vape smoke detected */
    VAPE_STATUS_STEAM_EXCLUDED,   /*!< Humidity steam excluded (no vape) */
    VAPE_STATUS_CHEMICAL_EXCLUDED /*!< Cleaning chemicals excluded (no vape) */
} vape_status_t;

/**
 * @brief Configurable thresholds for vape detection logic
 * 
 * Thuật toán xác định STATUS CHỈ LẤY CỦA PM2.5 (loại bỏ VOC, nhiệt độ, độ ẩm):
 * - Vượt ngưỡng PM25_VAPE_THRESHOLD (60 ug/m3) -> Báo động VAPE DETECTED.
 * - Hạ dưới PM25_CLEAR_THRESHOLD (45 ug/m3) -> Hủy báo động về CLEAN AIR.
 */
#define PM25_VAPE_THRESHOLD             60    /*!< PM2.5 threshold for Vape detection (ug/m3) */
#define PM25_CLEAR_THRESHOLD            45    /*!< PM2.5 threshold to clear Vape alert (ug/m3) */
#define PM25_CLEAN_THRESHOLD            15    /*!< PM2.5 threshold under which chemicals are checked (ug/m3) */
#define HUMIDITY_STEAM_THRESHOLD        90.0f /*!< Humidity threshold above which steam is active (RH%) */

#define VOC_DROP_SPIKE_THRESHOLD        800   /*!< SRAW_VOC drop relative to baseline to signal a VOC spike */
#define VOC_DROP_STABLE_THRESHOLD       300   /*!< SRAW_VOC drop below this is considered stable VOC */
#define VOC_DROP_CHEMICAL_THRESHOLD     2500  /*!< SRAW_VOC drop above this represents extreme VOCs (chemicals) */

#define VOC_EMA_ALPHA                   0.002f /*!< EMA decay coefficient for baseline tracking (~10 mins) */

/**
 * @brief Initialize the vape logic state
 */
void vape_logic_init(void);

/**
 * @brief Process new sensor readings and return current vape detection status
 * 
 * Vẫn đo đọc đầy đủ các cảm biến (temp, humid, VOC, NOx, PM2.5) để hiển thị OLED, BLE,
 * nhưng thuật toán ra quyết định status chỉ dựa trên PM2.5.
 * 
 * @param temp Current temperature in Celsius
 * @param humid Current relative humidity in %
 * @param sraw_voc Current raw VOC ticks from SGP41
 * @param sraw_nox Current raw NOx ticks from SGP41
 * @param pm2_5 Current PM2.5 concentration in ug/m3
 * @param voc_drop Output pointer to store calculated VOC drop from baseline
 * @param voc_baseline Output pointer to store current VOC baseline value
 * @return vape_status_t Current air classification status
 */
vape_status_t vape_logic_process(float temp, float humid, uint16_t sraw_voc, uint16_t sraw_nox, 
                                 uint16_t pm2_5, float *voc_drop, float *voc_baseline);

/**
 * @brief Convert vape status enum to human readable string
 * 
 * @param status Status enum
 * @return const char* String representation
 */
const char* vape_status_to_str(vape_status_t status);

#endif // VAPE_LOGIC_H
