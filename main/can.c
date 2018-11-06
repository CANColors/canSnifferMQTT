/* ========================================================================== */
/*                                                                            */
/*   Filename.c                                                               */
/*   (c) 2012 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/can.h"
#include <cjson.h>

#include "can.h"  
#include "timestamp.h"
#include "control.h"


/* --------------------- Definitions and static variables ------------------ */
//canSniffer Configuration
#define RX_TASK_PRIO                    9
#define TX_GPIO_NUM                     4
#define RX_GPIO_NUM                     5
#define CAN_EN_GPIO           22
#define RX_BUFFER_MAX                   20
#define CAN_TAG                     "CAN"


extern SemaphoreHandle_t can_rx;
extern SemaphoreHandle_t can_tx;
extern SemaphoreHandle_t http_rx;
extern SemaphoreHandle_t http_tx;
extern SemaphoreHandle_t http_tx_wait;

extern QueueHandle_t rxCanQueue;
extern QueueHandle_t txCanQueue;
extern QueueHandle_t controlEvents;

void testPrintQueue(void);


static CANSettings canCfg;

uint8_t cntRxCan = 0;


/* --------------------------- Tasks and Functions -------------------------- */

void can_receive_task(void *arg)
{
    xSemaphoreTake(can_rx, portMAX_DELAY);
   
   // 
   // testCanDataGenerate();
     ESP_LOGI(CAN_TAG, "Wait for CAN");    
    while (1) {
            
        can_message_t rx_msg;
        esp_err_t res = can_receive(&rx_msg, portMAX_DELAY);
        if (res == ESP_OK)
        {  
        ESP_LOGI(CAN_TAG, "REceived");
        can_msg_timestamped msg_timestamped;
        msg_timestamped.id = cntRxCan++;
        msg_timestamped.timestamp = get_timestamp();
        msg_timestamped.msg.identifier = rx_msg.identifier;
        msg_timestamped.msg.data_length_code = rx_msg.data_length_code;
        for (int i=0; i<rx_msg.data_length_code; i++)
        {
          msg_timestamped.msg.data[i] = rx_msg.data[i];
        }
              //printCanMessage(&rx_msg,iterations);
       ControlEvents cs2 = EV_CAN_REQUEST;    
       xQueueSend(controlEvents, &cs2, portMAX_DELAY);
       
       xQueueSend(rxCanQueue, &msg_timestamped, portMAX_DELAY);
      }
    }

    
    vTaskDelete(NULL);
}

void can_transmit_task(void *arg)
{
    can_msg_timestamped msg_timestamped;
    
    
   // xSemaphoreTake(tx_sem, portMAX_DELAY);
    xSemaphoreTake(can_tx, portMAX_DELAY); 
    while (1)
    {
      
       BaseType_t res =  xQueueReceive(txCanQueue, &msg_timestamped, portMAX_DELAY);
        if (res == pdTRUE)
        { //Тут ещё нужно прикрутить управаление временем отправки в соответствии с таймстампом.
          // Может кагда-то понадобится.
           ESP_LOGI(CAN_TAG, "Send to CAN");   
            msg_timestamped.msg.flags = CAN_MSG_FLAG_NONE;
             can_transmit(&msg_timestamped.msg, portMAX_DELAY);
             
             ControlEvents cs2 = EV_CAN_TRANSMITTED;    
             xQueueSend(controlEvents, &cs2, portMAX_DELAY);
        }
    
    }
    
 }  



static const can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();
static const can_timing_config_t t_config_500 = CAN_TIMING_CONFIG_500KBITS();
static const can_timing_config_t t_config_100 = CAN_TIMING_CONFIG_100KBITS();
//Set TX queue length to 0 due to listen only mode
static const can_general_config_t g_config = {.mode = CAN_MODE_NORMAL,
                                              .tx_io = TX_GPIO_NUM, .rx_io = RX_GPIO_NUM,
                                              .clkout_io = CAN_IO_UNUSED, .bus_off_io = CAN_IO_UNUSED,
                                              .tx_queue_len = 5, .rx_queue_len = 5,
                                              .alerts_enabled = CAN_ALERT_NONE,
                                              .clkout_divider = 0};


//---------------------------------------------------------------//
void canInit(void)
{
//Enable CAN bus output
    gpio_pad_select_gpio(CAN_EN_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(CAN_EN_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(CAN_EN_GPIO, 0);
    
  
   //static const can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();
    //static const can_timing_config_t t_config = CAN_TIMING_CONFIG_500KBITS();
    //Set TX queue length to 0 due to listen only mode
    canCfg.general_cfg.mode = CAN_MODE_NORMAL;
    canCfg.general_cfg.tx_io = TX_GPIO_NUM;
    canCfg.general_cfg.rx_io = RX_GPIO_NUM;
    canCfg.general_cfg.clkout_io = CAN_IO_UNUSED;
    canCfg.general_cfg.bus_off_io = CAN_IO_UNUSED;
    canCfg.general_cfg.tx_queue_len = 5;
    canCfg.general_cfg.rx_queue_len = 50;
    canCfg.general_cfg.alerts_enabled = CAN_ALERT_NONE;
    canCfg.general_cfg.clkout_divider = 0;

    canCfg.filter_cfg.acceptance_code = 0;
    canCfg.filter_cfg.acceptance_mask = 0xFFFFFFFF;
    canCfg.filter_cfg.single_filter = true; // CAN_FILTER_CONFIG_ACCEPT_ALL();

  /*  canCfg.timing_cfg.brp = 8;
    canCfg.timing_cfg.tseg_1 = 15;
    canCfg.timing_cfg.tseg_2 = 4;
    canCfg.timing_cfg.sjw = 3;
    canCfg.timing_cfg.triple_sampling = false; // CAN_TIMING_CONFIG_500KBITS();
    */

    canCfg.timing_cfg.brp = 40;
    canCfg.timing_cfg.tseg_1 = 15;
    canCfg.timing_cfg.tseg_2 = 4;
    canCfg.timing_cfg.sjw = 3;
    canCfg.timing_cfg.triple_sampling = false; // CAN_TIMING_CONFIG_100KBITS();



    //Install and start CAN driver
   // ESP_ERROR_CHECK(can_driver_install(&canCfg.general_cfg , &canCfg.filter_cfg, &canCfg.timing_cfg));
   // ESP_ERROR_CHECK(can_driver_install(&canCfg.general_cfg , &canCfg.timing_cfg, &canCfg.filter_cfg));

    ESP_ERROR_CHECK(can_driver_install(&g_config, &t_config_500, &f_config));
    ESP_LOGI(CAN_TAG, "Driver installed");
    ESP_ERROR_CHECK(can_start());
    ESP_LOGI(CAN_TAG, "Driver started");

     ESP_ERROR_CHECK(can_stop());
      ESP_ERROR_CHECK(can_driver_uninstall());

      ESP_ERROR_CHECK(can_driver_install(&g_config, &t_config_100, &f_config));
      ESP_LOGI(CAN_TAG, "Driver installed");
      ESP_ERROR_CHECK(can_start());
      ESP_LOGI(CAN_TAG, "Driver started");

  //  xSemaphoreGive(rx_sem);                     //Start RX task
}

//---------------------------------------------------------------//
//---------------------------------------------------------------//
void canReInit(void)
{
  ESP_LOGI(CAN_TAG, "ReInit");

 // ESP_ERROR_CHECK(can_stop());
  ESP_LOGI(CAN_TAG, "Driver stopped");
  //Install and start CAN driver
 // ESP_ERROR_CHECK(can_driver_uninstall());
  ESP_LOGI(CAN_TAG, "Driver uninstalled");

  gpio_pad_select_gpio(CAN_EN_GPIO);
  /* Set the GPIO as a push/pull output */
  gpio_set_direction(CAN_EN_GPIO, GPIO_MODE_OUTPUT);
  gpio_set_level(CAN_EN_GPIO, 1);

//Enable CAN bus output
    gpio_pad_select_gpio(CAN_EN_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(CAN_EN_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(CAN_EN_GPIO, 0);


    //Install and start CAN driver
   //  ESP_ERROR_CHECK(can_driver_install(&canCfg.general_cfg , &canCfg.timing_cfg, &canCfg.filter_cfg));
    //ESP_ERROR_CHECK(can_driver_install(&g_config, &t_config, &f_config));
    ESP_LOGI(CAN_TAG, "Driver installed");
  //  ESP_ERROR_CHECK(can_start());
    ESP_LOGI(CAN_TAG, "Driver started");

  //  xSemaphoreGive(rx_sem);                     //Start RX task
}

//---------------------------------------------------------------//


void parseCANConfig(cJSON* jdata)
{
   cJSON *timecfg = cJSON_GetObjectItem(jdata,"time");
   cJSON *filtercfg = cJSON_GetObjectItem(jdata,"filter");

   if (timecfg != NULL )
   {
   uint32_t freq = cJSON_GetObjectItem(timecfg,"freq")->valueint;

   ESP_ERROR_CHECK(can_stop());
   ESP_LOGI(CAN_TAG, "Driver stopped");
   //Install and start CAN driver
   ESP_ERROR_CHECK(can_driver_uninstall());
   ESP_LOGI(CAN_TAG, "Driver uninstalled");
 //  if (freq != NULL)
   {
   switch (freq)
     {
     case 25:
       {//CAN_TIMING_CONFIG_25KBITS();
         canCfg.timing_cfg.brp = 128;
         canCfg.timing_cfg.tseg_1 = 16;
         canCfg.timing_cfg.tseg_2 = 8;
         canCfg.timing_cfg.sjw = 3;
         canCfg.timing_cfg.triple_sampling = false;
         ESP_LOGI(CAN_TAG, "CAN:25");
         break;}
     case 50:
       {//CAN_TIMING_CONFIG_50KBITS();
         canCfg.timing_cfg.brp = 80;
         canCfg.timing_cfg.tseg_1 = 15;
         canCfg.timing_cfg.tseg_2 = 4;
         canCfg.timing_cfg.sjw = 3;
         canCfg.timing_cfg.triple_sampling = false;
           ESP_ERROR_CHECK(can_driver_install(&g_config, &t_config_100, &f_config));
          ESP_LOGI(CAN_TAG, "CAN:50");
         break;}
     case 100:
       {//CAN_TIMING_CONFIG_100KBITS();
         canCfg.timing_cfg.brp = 40;
         canCfg.timing_cfg.tseg_1 = 15;
         canCfg.timing_cfg.tseg_2 = 4;
         canCfg.timing_cfg.sjw = 3;
         canCfg.timing_cfg.triple_sampling = false;
         ESP_ERROR_CHECK(can_driver_install(&g_config, &t_config_100, &f_config));
         ESP_LOGI(CAN_TAG, "CAN:100");
         break;}
     case 125:
       {//CAN_TIMING_CONFIG_125KBITS();
         canCfg.timing_cfg.brp = 32;
         canCfg.timing_cfg.tseg_1 = 15;
         canCfg.timing_cfg.tseg_2 = 4;
         canCfg.timing_cfg.sjw = 3;
         canCfg.timing_cfg.triple_sampling = false;
          ESP_LOGI(CAN_TAG, "CAN:125");
         break;}
     case 250:
       {// CAN_TIMING_CONFIG_250KBITS();
         canCfg.timing_cfg.brp = 16;
         canCfg.timing_cfg.tseg_1 = 15;
         canCfg.timing_cfg.tseg_2 = 4;
         canCfg.timing_cfg.sjw = 3;
         canCfg.timing_cfg.triple_sampling = false;
          ESP_LOGI(CAN_TAG, "CAN:250");
         break;}
     case 500:
       {// CAN_TIMING_CONFIG_500KBITS()
         canCfg.timing_cfg.brp = 8;
         canCfg.timing_cfg.tseg_1 = 15;
         canCfg.timing_cfg.tseg_2 = 4;
         canCfg.timing_cfg.sjw = 3;
         canCfg.timing_cfg.triple_sampling = false;
          ESP_LOGI(CAN_TAG, "CAN:500");
         break;}
       case 800:
       {// CAN_TIMING_CONFIG_800KBITS()
         canCfg.timing_cfg.brp = 4;
         canCfg.timing_cfg.tseg_1 = 16;
         canCfg.timing_cfg.tseg_2 = 2;
         canCfg.timing_cfg.sjw = 3;
         canCfg.timing_cfg.triple_sampling = false;
          ESP_LOGI(CAN_TAG, "CAN:800");
         break;}
     case 1000:
       {// CAN_TIMING_CONFIG_1000KBITS();
         canCfg.timing_cfg.brp = 4;
         canCfg.timing_cfg.tseg_1 = 15;
         canCfg.timing_cfg.tseg_2 = 4;
         canCfg.timing_cfg.sjw = 3;
         canCfg.timing_cfg.triple_sampling = false;
          ESP_LOGI(CAN_TAG, "CAN:1000");
         break;}
     default:break;
     }
   }
   }

   if (filtercfg != NULL)
    {
      uint32_t acc_code = cJSON_GetObjectItem(filtercfg,"acc_code")->valueint;
      uint32_t acc_mask = cJSON_GetObjectItem(filtercfg,"acc_mask")->valueint;
      uint8_t st = cJSON_GetObjectItem(filtercfg,"single_filter")->valueint;

    //  canCfg.filter_cfg.acceptance_code = acc_code;
    //  canCfg.filter_cfg.acceptance_mask = acc_mask;
    //  canCfg.filter_cfg.single_filter = st;

    }

   ESP_ERROR_CHECK(can_start());
   ESP_LOGI(CAN_TAG, "Driver started");
}

void testCanDataGenerate(void)
{
  can_msg_timestamped msg_timestamped;
  
    for (int i=0;i<10;i++)
    {
        msg_timestamped.id = i;
        msg_timestamped.timestamp = get_timestamp();
        msg_timestamped.msg.identifier = 0x7DE;
        msg_timestamped.msg.data_length_code = 8;
        for (int j=0;j<8;j++)
        {
          msg_timestamped.msg.data[j] = i;
        }
        xQueueSend(rxCanQueue, &msg_timestamped, portMAX_DELAY);
       // vTaskDelay(50 / portTICK_PERIOD_MS);
    }
   
//  testPrintQueue();  

}
  
/*
void testPrintQueue(void)
{
    UBaseType_t cntMsg = uxQueueMessagesWaiting( rxCanQueue );
    ESP_LOGI("PrintQueue", "cntMsg =>  %d", cntMsg );

    can_msg_timestamped msg;
    char buf [50];
     BaseType_t res =  xQueueReceive(rxCanQueue, &msg, 0);
     ESP_LOGI("PrintQueue", "MsgId =>  %d", msg.id );
     sprintf(buf,"%ud", msg.timestamp);
     ESP_LOGI("PrintQueue", "MsgTimestamp =>  %d", msg.timestamp );
     ESP_LOGI("PrintQueue", "MsgAddress =>  %d", msg.msg.identifier );
     ESP_LOGI("PrintQueue", "MsgData0 =>  %d", msg.msg.data[0] );
     
   
}
     
  */     
 
