/* ========================================================================== */
/*                                                                            */
/*   Filename.c                                                               */
/*   (c) 2012 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */


  #define MQTT_REQUEST_TOPIC      "request"
  #define MQTT_RESPONSE_TOPIC     "responce"
  #define MQTT_HEARTBEAT_TOPIC    "heartbeat"
  
 
#define MQTT_SNIFFER_TOPIC        "sniffer"


#define CAN_RX_BUFFER   110
#define CAN_TX_BUFFER   10

#define TIME_HEARTBEAT        5000
#define NET_TRANSMIT_TIMESLOT     100

/* Constants that aren't configurable in menuconfig */
#define MQTT_URL "mqtt://192.168.137.102"
//#define MQTT_URL "mqtt://iot.eclipse.org",
#define MQTT_PORT 1883



#define APP_WIFI_SSID CONFIG_WIFI_SSID
#define APP_WIFI_PASS CONFIG_WIFI_PASSWORD