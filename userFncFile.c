#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "inc/userFncFile.h"
#include "inc/systemTimer.h"
#include "inc/led_Led1.h"
#include "inc/timer_api1.h"
#include "inc/timer_api2.h"
#include "inc/timer_api3.h"
#include "inc/timer_api4.h"
#include "inc/timer_api5.h"
#include "inc/timer_api6.h"
#include "inc/conversionFunctions.h"
#include "inc/EMICBus.h"
#include "inc/MQTT_Mqtt.h"
#include "inc/Persist.h"

void MQTT_Mqtt_onConnected()
{
    MQTT_Mqtt_publish("1", "Motor-controller/da1248d2-2283-4/upload/PROBE");
}



