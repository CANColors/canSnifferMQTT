/* ========================================================================== */
/*                                                                            */
/*   Filename.c                                                               */
/*   (c) 2012 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */
#include "driver/can.h"
#include <cjson.h>

typedef struct {
uint id;
uint32_t timestamp;
can_message_t msg;
}can_msg_timestamped;


typedef struct
{
  can_timing_config_t  timing_cfg;
  can_filter_config_t  filter_cfg;
  can_general_config_t general_cfg;
}CANSettings;




void canInit(void);
void canReInit(void);
void testCanDataGenerate(void);
void can_receive_task(void *arg);
void can_transmit_task(void *arg);
void parseCANConfig(cJSON* jdata);
