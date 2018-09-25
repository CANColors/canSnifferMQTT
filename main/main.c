/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>

#include <esp_err.h>
#include <esp_log.h>
#include <mqtt_client.h>


#include "config.h"
#include "can.h"
#include "wifi.h"
#include "mqtt.h"
#include "timestamp.h"
#include "control.h"


#define MQTT_RX_TASK_PRIO            8
#define MQTT_TX_TASK_PRIO            8
#define CAN_TX_TASK_PRIO          9
#define CAN_RX_TASK_PRIO          9

#define CONTROL_TASK_PRIO          9


#define CONTROL_QUEUE_MAX   10


/* A simple example that demonstrates how to create GET and POST
 * handlers for the web server.
 * The examples use simple WiFi configuration that you can set via
 * 'make menuconfig'.
 * If you'd rather not, just change the below entries to strings
 * with the config you want -
 * ie. #define EXAMPLE_WIFI_SSID "mywifissid"
*/


static const char *TAG="APP";

QueueHandle_t rxCanQueue;
QueueHandle_t txCanQueue;
QueueHandle_t controlQueue;
extern QueueHandle_t controlEvents;

SemaphoreHandle_t can_rx;
SemaphoreHandle_t can_tx;
SemaphoreHandle_t mqtt_rx;
SemaphoreHandle_t mqtt_tx;

uint16_t cntTransaction = 0;

//extern const int CONNECTED_BIT;


  
void app_main()
{
   
    ESP_ERROR_CHECK(nvs_flash_init());
    
     can_rx = xSemaphoreCreateBinary();
     can_tx = xSemaphoreCreateBinary();
     
     mqtt_rx = xSemaphoreCreateBinary();
     mqtt_tx = xSemaphoreCreateBinary();
     
    rxCanQueue = xQueueCreate( CAN_RX_BUFFER, sizeof( can_msg_timestamped) );
    txCanQueue = xQueueCreate( CAN_TX_BUFFER, sizeof( can_msg_timestamped) );
    
    controlQueue = xQueueCreate( CONTROL_QUEUE_MAX, sizeof( ControlState) );
     
    wifi_initialise(NULL);
    wifi_wait_connected();
    
    MqttInitM();
    set_time();
    canInit();
    
    testCanDataGenerate();
    
     /*
     xTaskCreatePinnedToCore(http_receive_task,  "HTTP_rx", 4096, NULL, HTTP_RX_TASK_PRIO, NULL, tskNO_AFFINITY);
     xTaskCreatePinnedToCore(http_transmit_task, "HTTP_tx", 4096, NULL, HTTP_TX_TASK_PRIO, NULL, tskNO_AFFINITY);
   
     xTaskCreatePinnedToCore(can_transmit_task, "CAN_tx", 4096, NULL, CAN_TX_TASK_PRIO, NULL, tskNO_AFFINITY);
     xTaskCreatePinnedToCore(can_receive_task,  "CAN_rx", 4096, NULL, CAN_RX_TASK_PRIO, NULL, tskNO_AFFINITY);
       */
       
   //  xTaskCreatePinnedToCore(mqtt_receive_task,  "MQTT_rx", 4096, NULL, MQTT_RX_TASK_PRIO, NULL, 0);
     xTaskCreatePinnedToCore(mqtt_transmit_task, "MQTT_tx", 4096, NULL, MQTT_TX_TASK_PRIO, NULL, 0);
   
     xTaskCreatePinnedToCore(can_transmit_task, "CAN_tx", 4096, NULL, CAN_TX_TASK_PRIO, NULL, 1);
     xTaskCreatePinnedToCore(can_receive_task,  "CAN_rx", 4096, NULL, CAN_RX_TASK_PRIO, NULL, 1);
   
     xTaskCreatePinnedToCore(control_task,  "control", 4096, NULL,  CONTROL_TASK_PRIO , NULL, tskNO_AFFINITY);
   
   ESP_LOGI(TAG, "Ready to communications" ); 
   
     ControlState cs2 = CONTROL_START;    //return to start
     xQueueSend(controlQueue, &cs2, portMAX_DELAY);
    
    //Test
    // cs2 = CONTROL_CAN_REQUEST_WAIT;   
    // xQueueSend(controlQueue, &cs2, portMAX_DELAY);
     
   //  ControlEvents ce = EV_CAN_REQUEST;    
   //    xQueueSend(controlEvents, &ce, portMAX_DELAY);
       
    // cs2 = CONTROL_SEND_REQUEST;   
    // xQueueSend(controlQueue, &cs2, portMAX_DELAY);
     
  // xSemaphoreGive(http_rx);
  
   
}
