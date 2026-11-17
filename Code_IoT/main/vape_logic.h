#ifndef VAPE_LOGIC_H
#define VAPE_LOGIC_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Detection status result from multi-sensor fusion
 */
typedef enum {
    VAPE_STATUS_CLEAN = 0,         /*!< Normal clean air */
    VAPE_STATUS_VAPE_DETECTED,    /*!< Vape smoke detected */
    VAPE_STATUS_STEAM_EXCLUDED,   /*!< Humidity steam excluded (no vape) */
    VAPE_STATUS_CHEMICAL_EXCLUDED /*!< Cleaning chemicals excluded (no vape) */
} vape_status_t;

/**
 * @brief Configurable thresholds for multi-sensor fusion logic
 */
#define PM25_RISE_VAPE_THRESHOLD        4     /*!< PM2.5 rise above baseline to detect Vape (ug/m3) */
#define PM25_CLEAN_THRESHOLD            15    /*!< PM2.5 absolute threshold under which chemicals are checked (ug/m3) */
#define HUMIDITY_STEAM_THRESHOLD        90.0f /*!< Humidity threshold above which steam is active (RH%) */

#define VOC_DROP_SPIKE_THRESHOLD        150   /*!< SRAW_VOC drop relative to baseline to signal a VOC spike (lowered for slow diffusion) */
#define VOC_DROP_STABLE_THRESHOLD       100   /*!< SRAW_VOC drop below this is considered stable VOC */
#define VOC_DROP_CHEMICAL_THRESHOLD     2500  /*!< SRAW_VOC drop above this represents extreme VOCs (chemicals) */

#define VOC_EMA_ALPHA                   0.002f /*!< EMA decay coefficient for baseline tracking (~10 mins) */

/**
 * @brief Initialize the vape logic state
 */
void vape_logic_init(void);

/**
 * @brief Set the dynamic PM2.5 baseline after conditioning
 */
void vape_logic_set_pm25_baseline(float pm25_base);

/**
 * @brief Process new sensor readings and return current vape detection status
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
