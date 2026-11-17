#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "esp_err.h"
#include "driver/i2c.h"

#define I2C_MASTER_SDA_IO           8       /*!< GPIO number for I2C master data  */
#define I2C_MASTER_SCL_IO           9       /*!< GPIO number for I2C master clock */
#define I2C_MASTER_NUM              I2C_NUM_0 /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ          100000  /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0       /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0       /*!< I2C master doesn't need buffer */

/**
 * @brief Initialize the I2C master bus
 * 
 * @return esp_err_t ESP_OK on success, or appropriate error code
 */
esp_err_t i2c_bus_init(void);

/**
 * @brief Write bytes to an I2C slave device
 * 
 * @param dev_addr Address of the target device
 * @param data Pointer to the buffer containing data to write
 * @param size Number of bytes to write
 * @return esp_err_t ESP_OK on success, or error code
 */
esp_err_t i2c_bus_write_bytes(uint8_t dev_addr, const uint8_t *data, size_t size);

/**
 * @brief Read bytes from an I2C slave device
 * 
 * @param dev_addr Address of the target device
 * @param data Pointer to the buffer where received data will be stored
 * @param size Number of bytes to read
 * @return esp_err_t ESP_OK on success, or error code
 */
esp_err_t i2c_bus_read_bytes(uint8_t dev_addr, uint8_t *data, size_t size);

/**
 * @brief Write a command and then read response bytes from an I2C device (write then read)
 * 
 * @param dev_addr Address of the target device
 * @param cmd Pointer to the command buffer
 * @param cmd_size Size of the command buffer
 * @param data Pointer to the receive buffer
 * @param data_size Number of bytes to read
 * @return esp_err_t ESP_OK on success, or error code
 */
esp_err_t i2c_bus_write_cmd_read_bytes(uint8_t dev_addr, const uint8_t *cmd, size_t cmd_size, uint8_t *data, size_t data_size);

#endif // I2C_BUS_H
