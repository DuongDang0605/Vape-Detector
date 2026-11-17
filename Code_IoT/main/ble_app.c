#include "ble_app.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// NimBLE headers
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "BLE_APP";

static uint16_t ble_app_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t ble_app_char_handle = 0;
static uint8_t ble_app_addr_type = 0;

static int ble_app_gap_event(struct ble_gap_event *event, void *arg);
static void ble_app_advertise(void);

// Custom BLE UUIDs: 
// Service FFE0: 0000ffe0-0000-1000-8000-00805f9b34fb (Little Endian representation)
static const ble_uuid128_t ble_app_svc_uuid = BLE_UUID128_INIT(
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xe0, 0xff, 0x00, 0x00
);

// Characteristic FFE1: 0000ffe1-0000-1000-8000-00805f9b34fb (Little Endian representation)
static const ble_uuid128_t ble_app_char_uuid = BLE_UUID128_INIT(
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xe1, 0xff, 0x00, 0x00
);

static int ble_app_gatt_char_access(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            ESP_LOGI(TAG, "GATT Read Request received");
            // Return empty value or dummy placeholder if read directly
            return 0;
        default:
            return BLE_ATT_ERR_UNLIKELY;
    }
}

// GATT Database structure definition
static const struct ble_gatt_svc_def ble_app_gatt_db[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &ble_app_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &ble_app_char_uuid.u,
                .access_cb = ble_app_gatt_char_access,
                .val_handle = &ble_app_char_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            },
            { 0 } // No more characteristics
        }
    },
    { 0 } // No more services
};

static void ble_app_advertise(void)
{
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    int rc;

    memset(&fields, 0, sizeof(fields));
    
    // Discoverable mode flags
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    
    // Set Device Name
    fields.name = (uint8_t *)"ESP32S3_VAPE";
    fields.name_len = strlen("ESP32S3_VAPE");
    fields.name_is_complete = 1;
    
    // Include 16-bit Service UUID in advertising packet
    static ble_uuid16_t uuid16 = BLE_UUID16_INIT(0xFFE0);
    fields.uuids16 = &uuid16;
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to set advertising fields: %d", rc);
        return;
    }

    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(ble_app_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_app_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to start advertising: %d", rc);
    } else {
        ESP_LOGI(TAG, "BLE advertising started (Device Name: ESP32S3_VAPE)");
    }
}

static int ble_app_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                ble_app_conn_handle = event->connect.conn_handle;
                ESP_LOGI(TAG, "BLE Client connected successfully (handle: %d)", ble_app_conn_handle);
            } else {
                ESP_LOGE(TAG, "BLE Connection failed; status: %d", event->connect.status);
                ble_app_advertise(); // Restart advertising
            }
            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "BLE Client disconnected (handle: %d). Restarting advertisement...", event->disconnect.conn.conn_handle);
            ble_app_conn_handle = BLE_HS_CONN_HANDLE_NONE;
            ble_app_advertise(); // Restart advertising
            return 0;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            ESP_LOGI(TAG, "Advertising complete; restarting...");
            ble_app_advertise();
            return 0;

        case BLE_GAP_EVENT_SUBSCRIBE:
            ESP_LOGI(TAG, "Client subscription updated: notify=%d, handle=%d", 
                     event->subscribe.cur_notify, event->subscribe.conn_handle);
            return 0;
            
        case BLE_GAP_EVENT_MTU:
            ESP_LOGI(TAG, "MTU size updated to %d bytes", event->mtu.value);
            return 0;

        default:
            return 0;
    }
}

static void ble_app_on_sync(void)
{
    int rc = ble_hs_id_infer_auto(0, &ble_app_addr_type);
    assert(rc == 0);
    
    // Start advertising
    ble_app_advertise();
}

static void ble_app_on_reset(int reason)
{
    ESP_LOGE(TAG, "NimBLE stack reset; reason=%d", reason);
}

static void ble_app_host_task(void *param)
{
    ESP_LOGI(TAG, "NimBLE host task started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

esp_err_t ble_app_init(void)
{
    // Initialize NVS (NimBLE requires NVS for bonding parameters, etc.)
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        return err;
    }

    // Initialize the NimBLE port
    int rc = nimble_port_init();
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to initialize NimBLE port");
        return ESP_FAIL;
    }

    // Configure sync and reset callbacks
    ble_hs_cfg.sync_cb = ble_app_on_sync;
    ble_hs_cfg.reset_cb = ble_app_on_reset;

    // Set GAP device name
    rc = ble_svc_gap_device_name_set("ESP32S3_VAPE");
    assert(rc == 0);

    // Initialize GAP and GATT services
    ble_svc_gap_init();
    ble_svc_gatt_init();

    // Register custom GATT service definitions
    rc = ble_gatts_count_cfg(ble_app_gatt_db);
    if (rc != 0) {
        return ESP_FAIL;
    }

    rc = ble_gatts_add_svcs(ble_app_gatt_db);
    if (rc != 0) {
        return ESP_FAIL;
    }

    // Spawn the FreeRTOS host thread
    nimble_port_freertos_init(ble_app_host_task);
    
    ESP_LOGI(TAG, "NimBLE BLE GATT Server initialized successfully");
    return ESP_OK;
}

void ble_app_send_data(const char *data)
{
    if (ble_app_conn_handle != BLE_HS_CONN_HANDLE_NONE && ble_app_char_handle != 0) {
        struct os_mbuf *om = ble_hs_mbuf_from_flat(data, strlen(data));
        if (om != NULL) {
            int rc = ble_gatts_notify_custom(ble_app_conn_handle, ble_app_char_handle, om);
            if (rc != 0) {
                ESP_LOGE(TAG, "Failed to send BLE notification: %d", rc);
            }
        }
    }
}
