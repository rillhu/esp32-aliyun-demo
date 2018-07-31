#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "iot_import.h"
#include "iot_export.h"

#define TAG "OTA-MQTT"
#define ota_mqtt_example CONFIG_OTA_MQTT_EXAMPLE
#define PRODUCT_KEY             "********************************"
#define DEVICE_NAME             "********************************"
#define DEVICE_SECRET           "********************************"

/* These are pre-defined topics */
#define TOPIC_UPDATE            "/"PRODUCT_KEY"/"DEVICE_NAME"/update"
#define TOPIC_ERROR             "/"PRODUCT_KEY"/"DEVICE_NAME"/update/error"
#define TOPIC_GET               "/"PRODUCT_KEY"/"DEVICE_NAME"/get"
#define TOPIC_DATA              "/"PRODUCT_KEY"/"DEVICE_NAME"/data"

#define MSG_LEN_MAX             (2048)
#define BUFFSIZE 6000

bool wifi_connected = 0;
bool ota_init_flag = 0;
const esp_partition_t * update_partition = NULL;
static int binary_file_length = 0;
static char ota_write_data[BUFFSIZE + 1] = { 0 };

#define EXAMPLE_TRACE(fmt, args...)  \
    do { \
        HAL_Printf("%s|%03d :: ", __func__, __LINE__); \
        HAL_Printf(fmt, ##args); \
        HAL_Printf("%s", "\r\n"); \
    } while(0)

void event_handle(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    uintptr_t packet_id = (uintptr_t)msg->msg;
    iotx_mqtt_topic_info_pt topic_info = (iotx_mqtt_topic_info_pt)msg->msg;

    switch (msg->event_type) {
    case IOTX_MQTT_EVENT_UNDEF:
        EXAMPLE_TRACE("undefined event occur.");
        break;

    case IOTX_MQTT_EVENT_DISCONNECT:
        EXAMPLE_TRACE("MQTT disconnect.");
        break;

    case IOTX_MQTT_EVENT_RECONNECT:
        EXAMPLE_TRACE("MQTT reconnect.");
        break;

    case IOTX_MQTT_EVENT_SUBCRIBE_SUCCESS:
        EXAMPLE_TRACE("subscribe success, packet-id=%u", (unsigned int)packet_id);
        break;

    case IOTX_MQTT_EVENT_SUBCRIBE_TIMEOUT:
        EXAMPLE_TRACE("subscribe wait ack timeout, packet-id=%u", (unsigned int)packet_id);
        break;

    case IOTX_MQTT_EVENT_SUBCRIBE_NACK:
        EXAMPLE_TRACE("subscribe nack, packet-id=%u", (unsigned int)packet_id);
        break;

    case IOTX_MQTT_EVENT_UNSUBCRIBE_SUCCESS:
        EXAMPLE_TRACE("unsubscribe success, packet-id=%u", (unsigned int)packet_id);
        break;

    case IOTX_MQTT_EVENT_UNSUBCRIBE_TIMEOUT:
        EXAMPLE_TRACE("unsubscribe timeout, packet-id=%u", (unsigned int)packet_id);
        break;

    case IOTX_MQTT_EVENT_UNSUBCRIBE_NACK:
        EXAMPLE_TRACE("unsubscribe nack, packet-id=%u", (unsigned int)packet_id);
        break;

    case IOTX_MQTT_EVENT_PUBLISH_SUCCESS:
        EXAMPLE_TRACE("publish success, packet-id=%u", (unsigned int)packet_id);
        break;

    case IOTX_MQTT_EVENT_PUBLISH_TIMEOUT:
        EXAMPLE_TRACE("publish timeout, packet-id=%u", (unsigned int)packet_id);
        break;

    case IOTX_MQTT_EVENT_PUBLISH_NACK:
        EXAMPLE_TRACE("publish nack, packet-id=%u", (unsigned int)packet_id);
        break;

    case IOTX_MQTT_EVENT_PUBLISH_RECVEIVED:
        EXAMPLE_TRACE("topic message arrived but without any related handle: topic=%.*s, topic_msg=%.*s",
                      topic_info->topic_len,
                      topic_info->ptopic,
                      topic_info->payload_len,
                      topic_info->payload);
        break;

    default:
        EXAMPLE_TRACE("Should NOT arrive here.");
        break;
    }
}
    
esp_ota_handle_t ota_init(void)
{
    esp_err_t err;
    esp_ota_handle_t update_handle = 0;


    ESP_LOGI(TAG, "Start initialize the ESP32 OTA");

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running) {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);

    update_partition = esp_ota_get_next_update_partition(NULL);
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);
    assert(update_partition != NULL);

    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed, error=%d", err);
    }
    ESP_LOGI(TAG, "esp_ota_begin succeeded");
    return update_handle;
}

int mqtt_client(void)
{
#define OTA_BUF_LEN        (5000)
    esp_err_t err;
    int rc = 0, ota_over = 0;
    void *pclient = NULL, *h_ota = NULL;
    iotx_conn_info_pt pconn_info;
    iotx_mqtt_param_t mqtt_params;
    char *msg_buf = NULL, *msg_readbuf = NULL;
    char buf_ota[OTA_BUF_LEN];
    esp_ota_handle_t update_handle = 0;

    if (NULL == (msg_buf = (char *)HAL_Malloc(MSG_LEN_MAX))) {
        EXAMPLE_TRACE("not enough memory");
        rc = -1;
        goto do_exit;
    }

    if (NULL == (msg_readbuf = (char *)HAL_Malloc(MSG_LEN_MAX))) {
        EXAMPLE_TRACE("not enough memory");
        rc = -1;
        goto do_exit;
    }

    /* Device AUTH */
    if (0 != IOT_SetupConnInfo(PRODUCT_KEY, DEVICE_NAME, DEVICE_SECRET, (void **)&pconn_info)) {
        EXAMPLE_TRACE("AUTH request failed!");
        rc = -1;
        goto do_exit;
    }

    /* Initialize MQTT parameter */
    memset(&mqtt_params, 0x0, sizeof(mqtt_params));

    mqtt_params.port = pconn_info->port;
    mqtt_params.host = pconn_info->host_name;
    mqtt_params.client_id = pconn_info->client_id;
    mqtt_params.username = pconn_info->username;
    mqtt_params.password = pconn_info->password;
    mqtt_params.pub_key = pconn_info->pub_key;

    mqtt_params.request_timeout_ms = 2000;
    mqtt_params.clean_session = 0;
    mqtt_params.keepalive_interval_ms = 60000;
    mqtt_params.pread_buf = msg_readbuf;
    mqtt_params.read_buf_size = MSG_LEN_MAX;
    mqtt_params.pwrite_buf = msg_buf;
    mqtt_params.write_buf_size = MSG_LEN_MAX;

    mqtt_params.handle_event.h_fp = event_handle;
    mqtt_params.handle_event.pcontext = NULL;


    /* Construct a MQTT client with specify parameter */
    pclient = IOT_MQTT_Construct(&mqtt_params);
    if (NULL == pclient) {
        EXAMPLE_TRACE("MQTT construct failed");
        rc = -1;
        goto do_exit;
    }
    h_ota = IOT_OTA_Init(PRODUCT_KEY, DEVICE_NAME, pclient);
    if (NULL == h_ota) {
        rc = -1;
        EXAMPLE_TRACE("initialize OTA failed");
        goto do_exit;
    }

    if (0 != IOT_OTA_ReportVersion(h_ota, "iotx_esp_1.0.0")) {
        rc = -1;
        EXAMPLE_TRACE("report OTA version failed");
        goto do_exit;
    }

    HAL_SleepMs(1000);

    while (1) {
        if (!ota_over) {
            uint32_t firmware_valid;

            EXAMPLE_TRACE("wait ota upgrade command....");

            /* handle the MQTT packet received from TCP or SSL connection */
            IOT_MQTT_Yield(pclient, 200);

            if (IOT_OTA_IsFetching(h_ota)) {
                uint32_t last_percent = 0, percent = 0;
                char version[128], md5sum[33];
                uint32_t len, size_downloaded, size_file;

                do {
                    len = IOT_OTA_FetchYield(h_ota, buf_ota, OTA_BUF_LEN, 1);
                    if (len > 0) {

                        if (!ota_init_flag) {
                            update_handle = ota_init();
                            ota_init_flag = 1;
                        }
                        memcpy(ota_write_data, buf_ota, len);

                        err = esp_ota_write( update_handle, (const void *)ota_write_data, len);
                        if (err != ESP_OK) {
                            ESP_LOGE(TAG, "Error: esp_ota_write failed! err=0x%x", err);
                        }
                        binary_file_length += len;
                        ESP_LOGI(TAG, "Have written image length %d", binary_file_length);

                    }

                    /* get OTA information */
                    IOT_OTA_Ioctl(h_ota, IOT_OTAG_FETCHED_SIZE, &size_downloaded, 4);
                    IOT_OTA_Ioctl(h_ota, IOT_OTAG_FILE_SIZE, &size_file, 4);
                    IOT_OTA_Ioctl(h_ota, IOT_OTAG_MD5SUM, md5sum, 33);
                    IOT_OTA_Ioctl(h_ota, IOT_OTAG_VERSION, version, 128);

                    last_percent = percent;
                    percent = (size_downloaded * 100) / size_file;
                    if (percent - last_percent > 0) {
                        IOT_OTA_ReportProgress(h_ota, percent, NULL);
                        IOT_OTA_ReportProgress(h_ota, percent, "hello");
                    }
                    IOT_MQTT_Yield(pclient, 100);
                } while (!IOT_OTA_IsFetchFinish(h_ota));

                IOT_OTA_Ioctl(h_ota, IOT_OTAG_CHECK_FIRMWARE, &firmware_valid, 4);
                if (0 == firmware_valid) {
                    EXAMPLE_TRACE("The firmware is invalid");
                } else {

                    if (esp_ota_end(update_handle) != ESP_OK) {
                        ESP_LOGE(TAG, "esp_ota_end failed!");
                    }
                    err = esp_ota_set_boot_partition(update_partition);
                    if (err != ESP_OK)
                        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed! err=0x%x", err);

                    EXAMPLE_TRACE("The firmware is valid");
                    ESP_LOGI(TAG, "Prepare to restart system!");
                    esp_restart();
                }

                ota_over = 1;
            }
            HAL_SleepMs(2000);
        }
    }

    HAL_SleepMs(200);

do_exit:

    if (NULL != h_ota) {
        IOT_OTA_Deinit(h_ota);
    }

    if (NULL != pclient) {
        IOT_MQTT_Destroy(&pclient);
    }

    if (NULL != msg_buf) {
        HAL_Free(msg_buf);
    }

    if (NULL != msg_readbuf) {
        HAL_Free(msg_readbuf);
    }

    return rc;
}

void mqtt_proc(void *pvParameter)
{
    ESP_LOGI(TAG, "MQTT client example begin");
    while (wifi_connected) {
        mqtt_client();
    }
}

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG, "Connecting to AP...");
        esp_wifi_connect();
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "Connected.");
        if (esp_get_free_heap_size() < 30 * 1024)
            esp_restart();
        xTaskCreate(&mqtt_proc, "mqtt_proc", 4096 * 4, NULL, 4, NULL);
        wifi_connected = 1;
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "Wifi disconnected, try to connect ...");
        esp_wifi_connect();
        break;

    default:
        break;
    }

    return ESP_OK;
}

void initialize_wifi(void)
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    wifi_config_t sta_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
            .bssid_set = false
        }
    };

    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &sta_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

#if ota_mqtt_example
void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // OTA app partition table has a smaller NVS partition size than the non-OTA
        // partition table. This size mismatch may cause NVS initialization to fail.
        // If this happens, we erase NVS partition and initialize NVS again.
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    IOT_OpenLog("mqtt");

    IOT_SetLogLevel(IOT_LOG_DEBUG);

    IOT_DumpMemoryStats(IOT_LOG_DEBUG);

    initialize_wifi();

}

#endif