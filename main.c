#include <xc.h>
#include "inc/systemConfig.h"

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
#include "inc/system.h"
#include "inc/userFncFile.h"

#include "system.c"

int main(void)
{
	initSystem();
	systemTimeInit();
	LEDs_Led1_init();
	EMICBus_init();
	MQTT_Mqtt_init();
	do
	{
		poll_EMICBus();
		MQTT_Mqtt_poll();
	}
	while(1);
}

