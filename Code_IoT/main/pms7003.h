#ifndef PMS7003_H
#define PMS7003_H

#include "esp_err.h"
#include "driver/uart.h"

#define PMS_UART_PORT               UART_NUM_1
#define PMS_UART_BAUD               9600
#define PMS_TX_GPIO                 17
#define PMS_RX_GPIO                 18
#define PMS_BUF_SIZE                1024

/**
 * @brief Structure to store parsed PM data from PMS7003
 */
typedef struct {
    uint16_t pm1_0_std;  /*!< PM1.0 Concentration (CF=1, standard particle) [ug/m3] */
    uint16_t pm2_5_std;  /*!< PM2.5 Concentration (CF=1, standard particle) [ug/m3] */
    uint16_t pm10_std;   /*!< PM10 Concentration (CF=1, standard particle) [ug/m3] */
    uint16_t pm1_0_atm;  /*!< PM1.0 Concentration (Atmospheric environment) [ug/m3] */
    uint16_t pm2_5_atm;  /*!< PM2.5 Concentration (Atmospheric environment) [ug/m3] */
    uint16_t pm10_atm;   /*!< PM10 Concentration (Atmospheric environment) [ug/m3] */
} pms7003_data_t;

/**
 * @brief Initialize the PMS7003 UART connection
 * 
 * @return esp_err_t ESP_OK on success, or error code
 */
esp_err_t pms7003_init(void);

/**
 * @brief Read and parse the latest PM data from PMS7003
 * 
 * This is a non-blocking read that consumes bytes from the UART ring buffer,
 * parses complete 32-byte frames, verifies checksums, and updates data.
 * 
 * @param data Pointer to a pms7003_data_t structure to store parsed metrics
 * @return esp_err_t ESP_OK on new valid data, ESP_ERR_NOT_FOUND if no valid frame found
 */
esp_err_t pms7003_read(pms7003_data_t *data);

#endif // PMS7003_H
