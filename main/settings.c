#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/can.h>
#include <cjson.h>

#include "settings.h"
#include "can.h"


typedef struct
{
  CANSettings can;
  //OBDSettings obd;


}Settings;


void parseCommand(cJSON* data)
{
  cJSON* canSettings = cJSON_GetObjectItem(data,"cansettings");
  if (canSettings !=NULL)
    {
      parseCANConfig(canSettings);
  //    canReInit();
    }

}
