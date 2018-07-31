#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "iot_import.h"
#include "iot_export.h"

#define TAG "MQTT"
#define mqtt_example CONFIG_MQTT_EXAMPLE
#define PRODUCT_KEY             ""
#define DEVICE_NAME             ""
#define DEVICE_SECRET           ""
/* These are pre-defined topics */
#define TOPIC_UPDATE            "/"PRODUCT_KEY"/"DEVICE_NAME"/update"
#define TOPIC_ERROR             "/"PRODUCT_KEY"/"DEVICE_NAME"/update/error"
#define TOPIC_GET               "/"PRODUCT_KEY"/"DEVICE_NAME"/get"
#define TOPIC_DATA              "/"PRODUCT_KEY"/"DEVICE_NAME"/data"

#define MSG_LEN_MAX             (2048)

bool wifi_connected = 0;
int success_num = 1;
#define EXAMPLE_TRACE(fmt, args...)  \
    do { \
        printf("%s|%03d :: ", __func__, __LINE__); \
        printf(fmt, ##args); \
        printf("%s", "\r\n"); \
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
        success_num++;
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

static void _demo_message_arrive(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    iotx_mqtt_topic_info_pt ptopic_info = (iotx_mqtt_topic_info_pt) msg->msg;

    /* print topic name and topic message */
    EXAMPLE_TRACE("----");
    EXAMPLE_TRACE("Topic: '%.*s' (Length: %d)",
                  ptopic_info->topic_len,
                  ptopic_info->ptopic,
                  ptopic_info->topic_len);
    EXAMPLE_TRACE("Payload: '%.*s' (Length: %d)",
                  ptopic_info->payload_len,
                  ptopic_info->payload,
                  ptopic_info->payload_len);
    EXAMPLE_TRACE("----");
}

int mqtt_client(void)
{
    /* this arry can store the sucessful publish number */
    int PUBLISH_SUC_NUM[4] = {0, 0, 0, 2};
    /* use this variable to choose which arry element to store publish number */
    uint16_t LOOPFLAG = 0;
    int rc = 0, msg_len, cnt = 0;
    void *pclient;
    iotx_conn_info_pt pconn_info;
    iotx_mqtt_param_t mqtt_params;
    iotx_mqtt_topic_info_t topic_msg;
    char msg_pub[128];
    char *msg_buf = NULL, *msg_readbuf = NULL;
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
    mqtt_params.keepalive_interval_ms = 10000;
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
    /* Subscribe the specific topic */
    rc = IOT_MQTT_Subscribe(pclient, TOPIC_DATA, IOTX_MQTT_QOS1, _demo_message_arrive, NULL);
    if (rc < 0) {
        IOT_MQTT_Destroy(&pclient);
        EXAMPLE_TRACE("IOT_MQTT_Subscribe() failed, rc = %d", rc);
        rc = -1;
        goto do_exit;
    }

    HAL_SleepMs(1000);
    /* Initialize topic information */
    memset(&topic_msg, 0x0, sizeof(iotx_mqtt_topic_info_t));
    strcpy(msg_pub, "message: hello! start!");
    topic_msg.qos = IOTX_MQTT_QOS1;
    topic_msg.retain = 0;
    topic_msg.dup = 0;
    topic_msg.payload = (void *)msg_pub;
    topic_msg.payload_len = strlen(msg_pub);

    rc = IOT_MQTT_Publish(pclient, TOPIC_DATA, &topic_msg);
    EXAMPLE_TRACE("rc = IOT_MQTT_Publish() = %d", rc);
    while (1) {
        if (wifi_connected) {
            printf("\n************************     %d    **********************\n", esp_get_free_heap_size());
            /* check the net if occur some problems */
            PUBLISH_SUC_NUM[LOOPFLAG] = success_num;
            LOOPFLAG++;
            if (LOOPFLAG == 4)
                LOOPFLAG = 0;
            if (PUBLISH_SUC_NUM[0] == PUBLISH_SUC_NUM[3]) {
                EXAMPLE_TRACE("The net is not stable, please check the router/AP!\n");
            }
            msg_len = snprintf(msg_pub, sizeof(msg_pub), "{\"attr_name\":\"temperature\", \"attr_value\":\"%d\"}", cnt++);
            if (msg_len < 0) {
                EXAMPLE_TRACE("Error occur! Exit program");
                rc = -1;
                break;
            }

            topic_msg.payload = (void *)msg_pub;
            topic_msg.payload_len = msg_len;

            rc = IOT_MQTT_Publish(pclient, TOPIC_DATA, &topic_msg);

            if (rc < 0) {
                EXAMPLE_TRACE("error occur when publish");
                rc = -1;
                break;
            }
            /* handle the MQTT packet received from TCP or SSL connection */
            IOT_MQTT_Yield(pclient, 200);
            HAL_SleepMs(3000);

        } else {

            IOT_MQTT_Unsubscribe(pclient, TOPIC_DATA);
            IOT_MQTT_Destroy(&pclient);
            HAL_Free(msg_buf);
            HAL_Free(msg_readbuf);
            vTaskDelete(NULL);
            break;
        }
    }

    IOT_MQTT_Unsubscribe(pclient, TOPIC_DATA);
    HAL_SleepMs(2000);
    IOT_MQTT_Destroy(&pclient);

do_exit:
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
        xTaskCreate(&mqtt_proc, "mqtt_proc", 4096 * 2, NULL, 2, NULL);
        wifi_connected = 1;
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "Wifi disconnected, try to connect ...");
        esp_wifi_connect();
        wifi_connected = 0;
        break;

    default:
        break;
    }

    return ESP_OK;
}

void initialize_wifi(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
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
#if mqtt_example
void app_main(void)
{
    IOT_OpenLog("mqtt");
    IOT_SetLogLevel(IOT_LOG_DEBUG);

    IOT_DumpMemoryStats(IOT_LOG_DEBUG);

    initialize_wifi();
}

#endif