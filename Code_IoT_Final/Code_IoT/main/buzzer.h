#ifndef BUZZER_H
#define BUZZER_H

#include <stdbool.h>
#include "esp_err.h"

#define BUZZER_GPIO                 38  /*!< GPIO pin connected to Buzzer I/O */

// MH-FMD uses an S8550 PNP transistor, which triggers on LOW level (Active-LOW = 0)
#define BUZZER_ACTIVE_LEVEL          0

/**
 * @brief Initialize the Buzzer GPIO pin
 * 
 * @return esp_err_t ESP_OK on success, or error code
 */
esp_err_t buzzer_init(void);

/**
 * @brief Turn the Buzzer ON or OFF
 * 
 * @param active true to sound the buzzer, false to turn it off
 */
void buzzer_set(bool active);

/**
 * @brief Beep for a specified duration in milliseconds
 * 
 * @param duration_ms Beep duration in ms
 */
void buzzer_beep(uint32_t duration_ms);

#endif // BUZZER_H
