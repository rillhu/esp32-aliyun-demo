# esp32-aliyun-demo  

## 1. 阿里云物联网套件简介

![](https://i.imgur.com/OqjGiQL.png)

阿里云物联网套件包括以下部分：  

> - IoT Hub  
>  为设备和物联网应用程序提供发布和接收消息的安全通道。
>   详情请参考 [IoT Hub](https://help.aliyun.com/document_detail/30548.html?spm=5176.doc30523.2.1.WtHk0t)
> - 安全认证&权限策略
> - 规则引擎
> - 设备影子

**esp32-aliyun-demo 移植下载:**  

    git clone --recursive https://github.com/espressif/esp32-aliyun-demo.git  
    git submodule update --init --recursive

## 2. framework  

```
esp32-aliyun-demo
├── build                                   //存放编译后生成的文件
├── esp-idf                                 //esp-idf
├── components                              //核心移植组件
│   └── esp32-aliyun                        //esp32-aliyun submodule
│       ├── component.mk                    //makefile 文件
│       ├── iotkit-embedded                 //阿里物联网套件
│       ├── platform                        //ESP32 适配平台，HAL 接口实现
│       │   ├── os
│       │   │   ├── esp32                
│       │   │   │   ├── HAL_OS_esp32.c      //ESP32平台相关
│       │   │   │   └── HAL_TCP_esp32.c     //tcp 相关
│       │   │   └── include                 //所有被提供的函数的声明
│       │   │       ├── iot_export.h
│       │   │       └── iot_import.h
│       │   └── ssl
│       │       └── mbedtls                 
│       │           └── HAL_TLS_mbedtls.c   //TLS实现
│       └── README.md
├── main                                    //demo main 文件夹     
│   ├── component.mk
│   ├── Kconfig.projbuild                   //menuconfig 用户配置文件
│   └── mqtt-example.c                      //入口文件
├── Makefile                                //编译入口 makefile
├── set_env.sh                              //一键搭建编译环境脚本(linux)
└── README.md
```  
在用户进行开发时：  
所有和 `iotkit-embedded` 中相关的功能函数均只需要调用 `iot_export.h` 和 `iot_import.h`两个头文件，无须关心其他头文件；  
所有和 `esp-idf` 相关的功能函数需要参考 `esp-idf` 具体实现。  

## 3. 硬件平台  

- 开发板：[ESP32-DevKitC 开发板](http://esp-idf.readthedocs.io/en/latest/hw-reference/modules-and-boards.html#esp32-core-board-v2-esp32-devkitc) 或 [ESP-WROVER-KIT 开发板](http://esp-idf.readthedocs.io/en/latest/hw-reference/modules-and-boards.html#esp-wrover-kit)

- 路由器/ Wi-Fi 热点：可以连接外网

## 4. 编译环境搭建( ubuntu 16.04)  

- 如果是在 ubuntu x64 下进行开发，则只需要在 PC 有网络连接时运行 `set_env.sh` 脚本一键完成编译环境的搭建。  

- 本 Demo 适配于 [esp-idf](https://github.com/espressif/esp-idf), 如果已经在开发环境下对 esp-idf 进行过配置，则无须运行 `set_env.sh` 脚本。
 

- 如果是在其他平台下进行开发，具体请参考 [Get Started](http://esp-idf.readthedocs.io/en/latest/get-started/index.html).

## 5. 配置  

#### 5.1 编译配置项说明  

- demo 选择：  
运行 `make menuconfig` -> `choose demo` -> 勾选所需编译运行的 demo 即可；

- 编辑连接配置项  
打开 `Makefile`, 可以看到以下几条选择功能，默认全部打开，无须进行更改，如果需要更改或取消相关功能，对相应项可进行改动。

        CFLAGS += -D IOTX_DEBUG
        CFLAGS += -D MQTT_DIRECT
        CFLAGS += -D MQTT_COMM_ENABLED
        CFLAGS += -D OTA_SIGNAL_CHANNEL=1
        CFLAGS += -D COAP_DTLS_SUPPORT

|配置选项          |含义             |
|:----------------:|:---------------:|
|IOTX_DEBUG        |指定编译 SDK 的版本类型, 支持 `debug`, `release`|
|MQTT_DIRECT       |是否用 MQTT 直连模式代替 HTTPS 三方认证模式做设备认证|
|MQTT_COMM_ENABLED |是否使能 MQTT 通道功能的总开关|
|OTA_SIGNAL_CHANNEL|可以选择 MQTT 或 COAP 作为 OTA 的通道|
|COAP_DTLS_SUPPORT |在使用 COAP 时是否关闭 DTLS|

#### 5.2 ESP32 Wi-Fi 联网配置  

运行 `make menuconfig` -> Demo Configuration -> 输入热点的 Wi-Fi SSID & Wi-Fi Password  

![](https://i.imgur.com/UOA8dm4.png)
#### 5.3 阿里云物联网套件控制台中创建设备  

- 在制台中创建设备

 登录[IoT 控制台](http://iot.console.aliyun.com), 创建产品及在产品下创建设备和 Topic 类, 具体步骤如下:

 - 创建产品, 可得 `ProductKey`, `ProductSecret` (*华东2站点无`ProductSecret`*)
 - 在产品下创建设备, 可得 `DeviceName`, `DeviceSecret`
 - 定义 Topic: `$(PRODUCT_KEY)/$(DEVICE_NAME)/data`, 并设置权限为: 设备具有发布与订阅 **(此步骤非常重要)**

 > **注意:** 请根据所选站点 (华东 2, 杭州) 创建相应的产品与设备.

 具体请参考[控制台使用手册](https://help.aliyun.com/document_detail/42714.html)文档中的`创建产品`, `添加设备`以及`获取设备 Topic`部分.

- 填充设备参数

 将`main/main.c`程序文件中的设备参数替换为您在控制台申请到的设备参数.

        // TODO: 在以下段落替换下列宏定义为你在IoT控制台申请到的设备信息
        #define PRODUCT_KEY             "*******************"
        #define DEVICE_NAME             "*******************"
        #define DEVICE_SECRET           "*******************"

#### 5.4 阿里云物联网套件控制台中设置 OTA 固件升级

提供的 OTA 为通过 MQTT 连接进行升级，工作的流程如下：  

![](https://i.imgur.com/rH3ofOH.png)

由于 OTA 默认是未开启功能的，所以需要在 IoT 控制台进行开启和新建固件等一系列相关操作：

![](https://i.imgur.com/yA4FfiI.png)


## 6. 编译运行

#### 6.1 mqtt-example

* 返回顶层目录
* 执行 make menuconfig 指令，选择相应 demo 和 烧写串口
* 执行 make 指令, 编译 esp32-aliyun-demo, 命令如下  

		make flash monitor

编译成功后, 开始进行烧写。  
样例程序的基本逻辑流程为:
> 1. ESP32 联网
> 2. 创建一个MQTT客户端
> 3. 订阅主题 `$(PRODUCT_KEY)/$(DEVICE_NAME)/data`
> 4. 循环向该主题发布消息

运行后打印输出：  

```
esp@esp:~/workspace/esp32-aliyun-demo$ make flash monitor
Flashing binaries to serial port /dev/ttyUSB0 (app at offset 0x10000)...
esptool.py v2.1
Connecting.....
Chip is ESP32D0WDQ6 (revision 0)

...

MONITOR
 
...

I (393) MQTT: Connecting to AP...
I (2623) wifi: n:11 0, o:11 0, ap:255 255, sta:11 0, prof:1
I (2623) wifi: state: init -> auth (b0)
I (2633) wifi: state: auth -> assoc (0)
I (2643) wifi: state: assoc -> run (10)
I (2673) wifi: connected with wwwtest, channel 11
I (3673) event: ip: 192.168.43.156, mask: 255.255.255.0, gw: 192.168.43.1
I (3673) MQTT: Connected.
I (3673) MQTT: MQTT client example begin

...

[inf] iotx_mc_init(1948): MQTT init success!
[inf] _ssl_client_init(164): Loading the CA root certificate ...
cert. version     : 3
serial number     : 04:00:00:00:00:01:15:4B:5A:C3:94
issuer name       : C=BE, O=GlobalSign nv-sa, OU=Root CA, CN=GlobalSign Root CA
subject name      : C=BE, O=GlobalSign nv-sa, OU=Root CA, CN=GlobalSign Root CA
issued  on        : 1998-09-01 12:00:00
expires on        : 2028-01-28 12:00:00
signed using      : RSA with SHA1
RSA key size      : 2048 bits
basic constraints : CA=true
key usage         : Key Cert Sign, CRL Sign
[inf] _ssl_parse_crt(132): crt content:451
[inf] _ssl_client_init(172):  ok (0 skipped)
[inf] TLSConnectNetwork(337): Connecting to /public.iot-as-mqtt.cn-shanghai.aliyuncs.com/1883...
[inf] TLSConnectNetwork(342):  ok
[inf] TLSConnectNetwork(347):   . Setting up the SSL/TLS structure...
[inf] TLSConnectNetwork(357):  ok
[inf] TLSConnectNetwork(392): Performing the SSL/TLS handshake...
[inf] TLSConnectNetwork(400):  ok
[inf] TLSConnectNetwork(404):   . Verifying peer X.509 certificate..
[inf] _real_confirm(81): certificate verification result: 0x00
[dbg] iotx_mc_connect(2263): start MQTT connection with parameters: clientid=9AhUIZuwEiy.esp32_01|securemode=-1,timestamp=2524608000000,signmethod=hmacsha1,gw=0|, username=wiunMmUYWebSJgayWwQx0010bad000, password=b94fdc7929a744ffa1145a18517424e1
[inf] iotx_mc_connect(2283): mqtt connect success!
```

#### 6.2 ota-mqtt-example  

* 返回顶层目录
* 执行 make menuconfig 指令，选择烧写串口，对 进行选择，注意一定  

![](https://i.imgur.com/y57uOmV.jpg)

![](https://i.imgur.com/oXcuzSr.jpg)

* 执行 make 指令, 编译 ota-mqtt-example, 命令如下  

		make erase_flash flash && make monitor   

编译成功后, 开始进行烧写。  
样例程序的基本逻辑流程为:

> 1 编译好固件A、固件B  
> 2 固件A烧录到设备中，并运行设备，上报版本号  
> 3 在IoT控制台上添加目标固件B，固件版本号填为version2.0，并推送升级  
> 4 设备端将收到云端推送的URL，并从该URL拉取固件，同时上报状态，并写入设备Flash中  
> 5 设备重启，此时设备运行新固件（即固件B），启动后向云端上报版本号 “version2.0”  
> 6 云端收到版本号 “version2.0”后，方认为OTA完成  

运行后打印输出：  
* 则终端会输出建立 MQTT 连接的过程，并且显示等待 OTA 命令：  

        [inf] iotx_device_info_init(40): device_info created successfully!
        [dbg] iotx_device_info_set(50): start to set device info!
        [dbg] iotx_device_info_set(64): device_info set successfully!
        ...  
        [inf] iotx_mc_init(1630): MQTT init success!
        [inf] _ssl_client_init(167): Loading the CA root certificate ...
        cert. version     : 3
        serial number     : 04:00:00:00:00:01:15:4B:5A:C3:94
        issuer name       : C=BE, O=GlobalSign nv-sa, OU=Root CA, CN=GlobalSign Root CA
        subject name      : C=BE, O=GlobalSign nv-sa, OU=Root CA, CN=GlobalSign Root CA
        issued  on        : 1998-09-01 12:00:00
        expires on        : 2028-01-28 12:00:00
        signed using      : RSA with SHA1
        RSA key size      : 2048 bits
        basic constraints : CA=true
        key usage         : Key Cert Sign, CRL Sign
        [inf] _ssl_parse_crt(135): crt content:451
        [inf] _ssl_client_init(175):  ok (0 skipped)
        [inf] TLSConnectNetwork(345): Connecting to /9AhUIZuwEiy.iot-as-mqtt.cn-shanghai.aliyuncs.com/1883...
        [inf] TLSConnectNetwork(350):  ok
        [inf] TLSConnectNetwork(355):   . Setting up the SSL/TLS structure...
        [inf] TLSConnectNetwork(365):  ok
        [inf] TLSConnectNetwork(400): Performing the SSL/TLS handshake...
        [inf] TLSConnectNetwork(408):  ok
        [inf] TLSConnectNetwork(412):   . Verifying peer X.509 certificate..
        [inf] _real_confirm(84): certificate verification result: 0x00
        ...
        [inf] iotx_mc_connect(1960): mqtt connect success!
        [inf] iotx_mc_subscribe(1365): mqtt subscribe success,topic = /ota/device/upgrade/9AhUIZuwEiy/esp32_ota!
        mqtt_client|225 :: wait ota upgrade command....
        [dbg] iotx_mc_cycle(1246): SUBACK
        event_handle|065 :: subscribe success, packet-id=1
        [dbg] iotx_mc_cycle(1237): PUBACK
        event_handle|089 :: publish success, packet-id=2
        mqtt_client|225 :: wait ota upgrade command....
        mqtt_client|225 :: wait ota upgrade command....  
        
* 此时在 IoT 控制台进行验证或者批量烧写操作，则会开始 OTA 过程，终端显示 ESP32 通过 MQTT 从控制台得到的 BIN 文件的 url， 然后开始下载和擦写 flash， 终端输出如下：  

![](https://i.imgur.com/VrQ6sWQ.jpg)

        [dbg] iotx_mc_cycle(1254): PUBLISH
        [dbg] iotx_mc_handle_recv_PUBLISH(1089): msg.id = | 0 |
        [dbg] iotx_mc_handle_recv_PUBLISH(1090): topicName = | /ota/device/upgrade/9AhUIZuwEiy/esp32_ota |
        [dbg] iotx_mc_handle_recv_PUBLISH(1100): delivering msg ...
        [dbg] iotx_mc_deliver_message(859): topic be matched
        [dbg] otamqtt_UpgrageCb(108): topic=/ota/device/upgrade/9AhUIZuwEiy/esp32_ota
        [dbg] otamqtt_UpgrageCb(109): len=1006, topic_msg={"code":"1000","data":{"size":128992,"version":"v2","url":"https://iotx-ota.oss-cn-shanghai.aliyuncs.com/ota/..."} ...
        [wrn] json_parse_name_value(163): Backup last_char since 1006 != 1105
        [wrn] json_parse_name_value(163): Backup last_char since 944 != 984
        [dbg] httpclient_common(783): host: 'iotx-ota.oss-cn-shanghai.aliyuncs.com', port: 443
        [inf] _ssl_client_init(167): Loading the CA root certificate ...
        ...
        [inf] _ssl_parse_crt(135): crt content:451
        [inf] _ssl_client_init(175):  ok (0 skipped)
        [inf] TLSConnectNetwork(345): Connecting to /iotx-ota.oss-cn-shanghai.aliyuncs.com/443...
        [inf] TLSConnectNetwork(350):  ok
        [inf] TLSConnectNetwork(355):   . Setting up the SSL/TLS structure...
        [inf] TLSConnectNetwork(365):  ok
        [inf] TLSConnectNetwork(400): Performing the SSL/TLS handshake...
        [inf] TLSConnectNetwork(408):  ok
        [inf] TLSConnectNetwork(412):   . Verifying peer X.509 certificate..
        [inf] _real_confirm(84): certificate verification result: 0x00
        [dbg] httpclient_send_header(314): REQUEST (Length: 986 Bytes)
        ...
        [dbg] httpclient_response_parse(668): Read header : x-oss-server-time: 116
        [dbg] httpclient_retrieve_content(425): Current data: 
        [dbg] httpclient_retrieve_content(524): Total-Payload: 128992 Bytes; Read: 0 Bytes
        [inf] httpclient_recv(381): 255 bytes has been read
        [inf] httpclient_recv(381): 255 bytes has been read
        [inf] httpclient_recv(381): 255 bytes has been read
        
循环擦写直到将 BIN 文件传输完毕，ESP32 进行重启。  

        mqtt_client|282 :: The firmware is valid
        I (32261) OTA-MQTT: Prepare to restart system!  
        
至此，OTA 升级完毕。

*注意： 在升级完成后需要向控制台上报新的版本号，否则虽然升级完成但是控制台会显示升级失败。*






