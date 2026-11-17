#ifndef BLE_APP_H
#define BLE_APP_H

#include "esp_err.h"

/**
 * @brief Initialize BLE GATT Server and start advertising
 * 
 * @return esp_err_t ESP_OK on success, or error code
 */
esp_err_t ble_app_init(void);

/**
 * @brief Send sensor data JSON/string to the connected BLE client (Web app)
 * 
 * @param data Null-terminated JSON or raw CSV text string
 */
void ble_app_send_data(const char *data);

#endif // BLE_APP_H
