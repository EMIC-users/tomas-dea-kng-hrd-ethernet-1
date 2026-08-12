#include <xc.h>
#include "inc/led_Led1.h"
#include "inc/gpio.h"
#include "inc/systemTimer.h"

void LEDs_Led1_init (void)
{
	HAL_GPIO_PinCfg(Led1, GPIO_OUTPUT);
}




