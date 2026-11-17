#ifndef SGP41_H
#define SGP41_H

#include "esp_err.h"

#define SGP41_I2C_ADDR                  0x59    /*!< SGP41 I2C address */
#define SGP41_CMD_EXEC_COND             0x2612  /*!< Execute conditioning command */
#define SGP41_CMD_MEAS_RAW              0x2619  /*!< Measure raw VOC and NOx command */
#define SGP41_CMD_GET_SERIAL            0x3682  /*!< Get serial number command */

#define SGP41_DEFAULT_RH_TICKS          0x8000  /*!< Default RH ticks (50% RH) */
#define SGP41_DEFAULT_T_TICKS           0x6666  /*!< Default T ticks (25 degC) */

/**
 * @brief Initialize SGP41 sensor
 * 
 * @return esp_err_t ESP_OK on success, or error code
 */
esp_err_t sgp41_init(void);

/**
 * @brief Execute the conditioning phase for the SGP41 sensor (recommended on startup)
 * 
 * @param raw_temp Raw temperature ticks from SHT3x
 * @param raw_humid Raw humidity ticks from SHT3x
 * @param sraw_voc Output pointer to store VOC raw signal during conditioning
 * @return esp_err_t ESP_OK on success, or error code
 */
esp_err_t sgp41_execute_conditioning(uint16_t raw_temp, uint16_t raw_humid, uint16_t *sraw_voc);

/**
 * @brief Measure raw VOC and NOx ticks from SGP41 with temperature and humidity compensation
 * 
 * @param raw_temp Raw temperature ticks from SHT3x
 * @param raw_humid Raw humidity ticks from SHT3x
 * @param sraw_voc Output pointer to store raw VOC ticks
 * @param sraw_nox Output pointer to store raw NOx ticks
 * @return esp_err_t ESP_OK on success, or error code
 */
esp_err_t sgp41_measure_raw(uint16_t raw_temp, uint16_t raw_humid, uint16_t *sraw_voc, uint16_t *sraw_nox);

#endif // SGP41_H
