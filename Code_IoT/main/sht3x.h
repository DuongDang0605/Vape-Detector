#ifndef SHT3X_H
#define SHT3X_H

#include "esp_err.h"

#define SHT3X_I2C_ADDR             0x44    /*!< SHT31-D I2C address */
#define SHT3X_CMD_MEAS_CLOCK_STR   0x2C06  /*!< Measurement: Clock stretching enabled, high repeatability */
#define SHT3X_CMD_MEAS_NO_STR      0x2400  /*!< Measurement: Clock stretching disabled, high repeatability */

/**
 * @brief Initialize SHT3x sensor
 * 
 * @return esp_err_t ESP_OK on success, or error code
 */
esp_err_t sht3x_init(void);

/**
 * @brief Read temperature and humidity from SHT3x sensor
 * 
 * @param temp Pointer to store calculated temperature in Celsius
 * @param humid Pointer to store calculated relative humidity in percent
 * @param raw_temp Pointer to store raw temperature ticks (useful for SGP41 compensation)
 * @param raw_humid Pointer to store raw humidity ticks (useful for SGP41 compensation)
 * @return esp_err_t ESP_OK on success, or error code
 */
esp_err_t sht3x_read_temp_humid(float *temp, float *humid, uint16_t *raw_temp, uint16_t *raw_humid);

/**
 * @brief Calculate CRC-8 for Sensirion sensors
 * 
 * @param data Pointer to the buffer
 * @param len Length of the buffer
 * @return uint8_t CRC value
 */
uint8_t sensirion_crc8(const uint8_t *data, size_t len);

#endif // SHT3X_H
