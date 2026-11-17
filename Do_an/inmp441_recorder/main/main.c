#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_crt_bundle.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "i2s_mic.h"
#include "sd_wav.h"
#include "oled_display.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "RECORDER";

/* ========== CẤU HÌNH ========== */
#define BUTTON_PIN      GPIO_NUM_3
#define DEBOUNCE_MS     500

#define WIFI_SSID       "LAB KHUON MAU 303M"
#define WIFI_PASS       "3cslabsince2015"
#define SERVER_URL      "https://vuchuan1702-heart-ai-processor.hf.space/predict"
#define HTTP_TIMEOUT_MS 60000 
#define CHUNK_SAMPLES   MIC_DMA_BUF_LEN
#define QUEUE_DEPTH     16
static void mic_task(void *arg);
static void sd_task(void *arg);
static void upload_task(void *arg);
static void upload_wav(const char *filepath);
/* WiFi event group */
#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1
static EventGroupHandle_t s_wifi_eg;
#define OLED_LOCK()    xSemaphoreTake(s_i2c_mutex, portMAX_DELAY)
#define OLED_UNLOCK()  xSemaphoreGive(s_i2c_mutex)
typedef struct {
    int16_t data[CHUNK_SAMPLES];
    size_t  len;
} audio_chunk_t;

static audio_chunk_t  s_chunk_pool[QUEUE_DEPTH + 1];
static QueueHandle_t  s_free_queue;
static QueueHandle_t  s_data_queue;

static volatile bool     s_recording    = false;
static SemaphoreHandle_t s_i2c_mutex = NULL;
static volatile uint32_t s_elapsed_sec  = 0;
static char              s_last_file[64] = {0};  /* path file WAV vừa ghi xong */

/* ──────────── WiFi ──────────── */
static void wifi_event_handler(void *arg, esp_event_base_t base,
                                int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi mat ket noi, thu lai...");
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *e = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "WiFi OK — IP: " IPSTR, IP2STR(&e->ip_info.ip));
        xEventGroupSetBits(s_wifi_eg, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init(void)
{
    s_wifi_eg = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID,  &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,   IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    wifi_config_t wcfg = {
        .sta = {
            .ssid     = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wcfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Dang ket noi WiFi...");
    xEventGroupWaitBits(s_wifi_eg, WIFI_CONNECTED_BIT,
                        pdFALSE, pdTRUE, pdMS_TO_TICKS(15000));
}

/* ──────────── Upload WAV lên server ──────────── */
static void upload_wav(const char *filepath)
{
    ESP_LOGI(TAG, ">>> Đang gửi file (Streaming mode): %s", filepath);

    FILE *f = fopen(filepath, "rb");
    if (!f) {
        ESP_LOGE(TAG, "Lỗi: Không mở được file từ SD!");
        return;
    }

    // 1. Lấy kích thước file thực tế
    fseek(f, 0, SEEK_END);
    int file_size = ftell(f);
    rewind(f);

    // 2. Chuẩn bị Header và Footer cho Multipart
    const char *boundary = "----ESP32Boundary123";
    char header[256];
    int header_len = snprintf(header, sizeof(header),
        "--%s\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"heart_rec.wav\"\r\n"
        "Content-Type: audio/wav\r\n\r\n", boundary);

    char footer[64];
    int footer_len = snprintf(footer, sizeof(footer), "\r\n--%s--\r\n", boundary);

    // 3. Cấu hình HTTP Client
    esp_http_client_config_t http_cfg = {
        .url = SERVER_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = HTTP_TIMEOUT_MS,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .keep_alive_enable = true,
    };
    esp_http_client_handle_t client = esp_http_client_init(&http_cfg);

    char content_type[128];
    snprintf(content_type, sizeof(content_type), "multipart/form-data; boundary=%s", boundary);
    esp_http_client_set_header(client, "Content-Type", content_type);

    // 4. MỞ KẾT NỐI VÀ GỬI THEO LUỒNG (Streaming)
    // Tổng dung lượng = header + file + footer
    esp_err_t err = esp_http_client_open(client, header_len + file_size + footer_len);
    
    if (err == ESP_OK) {
        // Gửi Header
        esp_http_client_write(client, header, header_len);

        // Gửi File theo từng khối 1KB để không cháy RAM
        uint8_t *buffer = malloc(1024);
        if (buffer) {
            int read_bytes;
            while ((read_bytes = fread(buffer, 1, 1024, f)) > 0) {
                esp_http_client_write(client, (char *)buffer, read_bytes);
            }
            free(buffer);
        }

        // Gửi Footer
        esp_http_client_write(client, footer, footer_len);

        // 5. Đọc phản hồi từ Server
        esp_http_client_fetch_headers(client);
        int status_code = esp_http_client_get_status_code(client);
        
        if (status_code == 200) {
            char resp_buf[512];
            int len = esp_http_client_read_response(client, resp_buf, sizeof(resp_buf) - 1);
            if (len > 0) {
                resp_buf[len] = 0;
                ESP_LOGI(TAG, "AI Phản hồi: %s", resp_buf);
            }
        } else {
            ESP_LOGE(TAG, "Server lỗi: HTTP %d", status_code);
        }
    } else {
        ESP_LOGE(TAG, "Kết nối thất bại: %s", esp_err_to_name(err));
    }

    // 6. Dọn dẹp
    fclose(f);
    esp_http_client_cleanup(client);
    ESP_LOGI(TAG, "<<< Upload kết thúc.");
}
/* ──────────── Task bọc để chạy Upload ──────────── */
static void upload_task(void *arg)
{
    
    char *path = (char *)arg;
    if (path != NULL) {
        ESP_LOGI(TAG, "Task upload bắt đầu: %s", path);
        upload_wav(path); // Gọi hàm xử lý gửi dữ liệu lên Hugging Face
        free(path);       // Giải phóng bộ nhớ đường dẫn đã cấp phát ở sd_task
    }
    ESP_LOGI(TAG, "Task upload kết thúc.");
    vTaskDelete(NULL);    // Tự giải phóng task
}
static void mic_task(void *arg)
{
    ESP_LOGI(TAG, ">>> Mic task đang chạy...");
    while (1) {
        // Đợi cho đến khi biến s_recording được kích hoạt (bằng nút nhấn)
        while (!s_recording) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        ESP_LOGI(TAG, "Bắt đầu thu âm...");
        while (s_recording) {
            audio_chunk_t *chunk = NULL;
            // Lấy một ô nhớ trống từ hàng đợi free_queue
            if (xQueueReceive(s_free_queue, &chunk, pdMS_TO_TICKS(100)) == pdTRUE) {
                size_t got = 0;
                // Đọc dữ liệu thực tế từ driver I2S
                esp_err_t ret = i2s_mic_read(chunk->data, CHUNK_SAMPLES, &got, 200);
                
                if (ret == ESP_OK && got > 0) {
                    chunk->len = got;
                    // Gửi ô nhớ đã có dữ liệu sang hàng đợi data_queue để SD task xử lý
                    xQueueSend(s_data_queue, &chunk, portMAX_DELAY);
                } else {
                    xQueueSend(s_free_queue, &chunk, 0);
                }
            }
        }

        // Gửi tín hiệu kết thúc (sentinel) cho SD task
        audio_chunk_t *sentinel = NULL;
        if (xQueueReceive(s_free_queue, &sentinel, pdMS_TO_TICKS(500)) == pdTRUE) {
            sentinel->len = 0;
            xQueueSend(s_data_queue, &sentinel, portMAX_DELAY);
        }
        ESP_LOGI(TAG, "Dừng thu âm.");
    }
}
/* ──────────── Task ghi SD ──────────── */
static void sd_task(void *arg)
{
    while (1) {
        audio_chunk_t *chunk = NULL;
        while (1) {
            xQueueReceive(s_data_queue, &chunk, portMAX_DELAY);
            if (chunk->len > 0) break;
            xQueueSend(s_free_queue, &chunk, 0);
        }

        esp_err_t ret = sd_wav_open(MIC_SAMPLE_RATE, 1);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Khong mo file WAV!");
            s_recording = false;
            xQueueSend(s_free_queue, &chunk, 0);
            continue;
        }

        size_t total = 0;
        sd_wav_write(chunk->data, chunk->len);
        total += chunk->len;
        xQueueSend(s_free_queue, &chunk, 0);

        while (1) {
            chunk = NULL;
            xQueueReceive(s_data_queue, &chunk, portMAX_DELAY);
            if (chunk->len == 0) {
                xQueueSend(s_free_queue, &chunk, 0);
                break;
            }
            sd_wav_write(chunk->data, chunk->len);
            total += chunk->len;
            xQueueSend(s_free_queue, &chunk, 0);
        }

        sd_wav_close();
        ESP_LOGI(TAG, "<<< DUNG GHI xong");

        /* Lấy đường dẫn file vừa ghi → spawn upload task */
        sd_wav_get_last_path(s_last_file, sizeof(s_last_file));
        char *path_copy = malloc(strlen(s_last_file) + 1);
        if (path_copy) {
            strcpy(path_copy, s_last_file);
            xTaskCreate(upload_task, "upload", 12288, path_copy, 3, NULL);
        }
    }
}

/* ──────────── Task OLED ──────────── */
static void oled_task(void *arg)
{
    /* Hiển thị màn hình chờ lúc khởi động */
    OLED_LOCK();
    oled_show_ready();
    OLED_UNLOCK();
 
    bool      was_recording = false;
    TickType_t rec_start    = 0;
    char       filename[32] = {0};
 
    while (1) {
        bool rec = s_recording;
 
        if (rec && !was_recording) {
            /* Vừa bắt đầu ghi */
            rec_start     = xTaskGetTickCount();
            s_elapsed_sec = 0;
            was_recording = true;
 
            /* Lấy tên file hiện tại để hiển thị */
            if (s_last_file[0]) {
                /* Chỉ lấy phần tên file, bỏ /sdcard/ */
                const char *slash = strrchr(s_last_file, '/');
                strncpy(filename, slash ? slash + 1 : s_last_file,
                        sizeof(filename) - 1);
            }
 
        } else if (!rec && was_recording) {
            /* Vừa dừng ghi */
            was_recording = false;
            OLED_LOCK();
            oled_show_stopped(s_elapsed_sec);
            OLED_UNLOCK();
 
        } else if (rec) {
            /* Đang ghi — cập nhật timer */
            s_elapsed_sec = (xTaskGetTickCount() - rec_start)
                            / configTICK_RATE_HZ;
            OLED_LOCK();
            oled_show_recording(s_elapsed_sec,
                                filename[0] ? filename : NULL);
            OLED_UNLOCK();
        }
 
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ──────────── Task nút nhấn ──────────── */
static void button_task(void *arg)
{
    bool last_stable = true;
    bool last_raw    = true;
    TickType_t last_change = 0;

    while (1) {
        bool cur = gpio_get_level(BUTTON_PIN);
        if (cur != last_raw) {
            last_raw    = cur;
            last_change = xTaskGetTickCount();
        }
        if ((xTaskGetTickCount() - last_change) >= pdMS_TO_TICKS(DEBOUNCE_MS)) {
            if (last_raw != last_stable) {
                last_stable = last_raw;
                if (!last_stable) {
                    s_recording = !s_recording;
                    ESP_LOGI(TAG, "[BTN] %s", s_recording ? "BAT DAU" : "DUNG");
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ──────────── app_main ──────────── */
void app_main(void)
{
    ESP_LOGI(TAG, "=== INMP441 Recorder + Upload ===");

    /* NVS (bắt buộc cho WiFi) */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    /* Queue */
    s_free_queue = xQueueCreate(QUEUE_DEPTH + 1, sizeof(audio_chunk_t *));
    s_data_queue = xQueueCreate(QUEUE_DEPTH + 1, sizeof(audio_chunk_t *));
    for (int i = 0; i < QUEUE_DEPTH + 1; i++) {
        audio_chunk_t *p = &s_chunk_pool[i];
        xQueueSend(s_free_queue, &p, 0);
    }

    /* GPIO nút */
    gpio_config_t btn = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&btn));

    /* Khởi tạo peripherals */
    ESP_ERROR_CHECK(i2s_mic_init());
    ESP_ERROR_CHECK(sd_wav_init());
    ESP_ERROR_CHECK(oled_init());

    /* WiFi */
    wifi_init();
    s_i2c_mutex = xSemaphoreCreateMutex();
    assert(s_i2c_mutex != NULL);

    /* Tasks */
    xTaskCreate(mic_task,    "mic",    8192, NULL, 6,  NULL);
    xTaskCreate(sd_task,     "sd",     8192, NULL, 4,  NULL);
    xTaskCreate(button_task, "button", 2048, NULL, 10, NULL);
    xTaskCreate(oled_task,   "oled",   8192, NULL, 3,  NULL);

    ESP_LOGI(TAG, "San sang. Nhan GPIO%d de ghi...", BUTTON_PIN);
}