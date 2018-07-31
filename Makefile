#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := main

#
# choose different Macro definition to enable diferent function
# ------------------------------------------------------------------
# |    IOTX_DEBUG     |             open debug log                 |
# ------------------------------------------------------------------
# |MQTT_DIRECT        |use mqtt direct connect instead through http|
# ------------------------------------------------------------------
# | MQTT_COMM_ENABLED |                open MQTT                   |
# ------------------------------------------------------------------
# | OTA_SIGNAL_CHANNEL|        = 1(use mqtt) = 2(use coap)         |
# ------------------------------------------------------------------
# | COAP_DTLS_SUPPORT |            use DTLS with coap              |
# ------------------------------------------------------------------
#

CFLAGS += -D IOTX_DEBUG

CFLAGS += -D MQTT_COMM_ENABLED

CFLAGS += -D OTA_SIGNAL_CHANNEL=1

CFLAGS += -D COAP_DTLS_SUPPORT

CFLAGS += -D MQTT_DIRECT

COMPONENT_ADD_INCLUDEDIRS := components/esp32-aliyun/platform/os/include

include $(IDF_PATH)/make/project.mk
