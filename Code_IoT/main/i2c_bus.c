#include "i2c_bus.h"
#include "esp_log.h"

static const char *TAG = "I2C_BUS";

esp_err_t i2c_bus_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C configuration failed with error: %s", esp_err_to_name(err));
        return err;
    }

    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver installation failed with error: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "I2C master initialized successfully (SDA: GPIO%d, SCL: GPIO%d, Freq: %dHz)", 
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, I2C_MASTER_FREQ_HZ);
    return ESP_OK;
}

esp_err_t i2c_bus_write_bytes(uint8_t dev_addr, const uint8_t *data, size_t size)
{
    if (data == NULL || size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_master_write_to_device(I2C_MASTER_NUM, dev_addr, data, size, pdMS_TO_TICKS(100));
}

esp_err_t i2c_bus_read_bytes(uint8_t dev_addr, uint8_t *data, size_t size)
{
    if (data == NULL || size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_master_read_from_device(I2C_MASTER_NUM, dev_addr, data, size, pdMS_TO_TICKS(100));
}

esp_err_t i2c_bus_write_cmd_read_bytes(uint8_t dev_addr, const uint8_t *cmd, size_t cmd_size, uint8_t *data, size_t data_size)
{
    if (cmd == NULL || cmd_size == 0 || data == NULL || data_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_master_write_read_device(I2C_MASTER_NUM, dev_addr, cmd, cmd_size, data, data_size, pdMS_TO_TICKS(100));
}
