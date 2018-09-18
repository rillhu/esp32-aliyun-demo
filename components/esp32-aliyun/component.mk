#
# Component Makefile
#

ifndef IOTX_DEBUG
CFLAGS += -D IOTX_DEBUG
CFLAGS += -D OTA_SIGNAL_CHANNEL=1
CFLAGS += -D MQTT_COMM_ENABLED
endif
COMPONENT_ADD_INCLUDEDIRS := platform/os/include \
							iotkit-embedded/src/tfs/platform \
							iotkit-embedded/src/sdk-impl/exports \
							iotkit-embedded/src/sdk-impl/imports \
							iotkit-embedded/src/mqtt \
							iotkit-embedded/src/sdk-impl \
							iotkit-embedded/src/packages/LITE-log \
							iotkit-embedded/src/packages/LITE-utils \
							iotkit-embedded/src/packages/LITE-utils \
							iotkit-embedded/src/utils/digest \
							iotkit-embedded/src/utils/misc \
							iotkit-embedded/src/system \
							iotkit-embedded/src/guider \
							iotkit-embedded/src/security

COMPONENT_SRCDIRS := platform/os/esp32 \
					iotkit-embedded/src/tfs/platform \
					iotkit-embedded/src \
					platform/ssl/mbedtls \
					iotkit-embedded/src/guider \
					iotkit-embedded/src/mqtt \
					iotkit-embedded/src/ota \
					iotkit-embedded/src/scripts \
					iotkit-embedded/src/security \
					iotkit-embedded/src/shadow \
					iotkit-embedded/src/system \
					iotkit-embedded/src/packages/LITE-utils \
					iotkit-embedded/src/packages/LITE-log \
					iotkit-embedded/src/packages \
					iotkit-embedded/src/utils/digest \
					iotkit-embedded/src/utils/misc \
					iotkit-embedded/src/sdk-impl \
					iotkit-embedded/src/mqtt/MQTTPacket
