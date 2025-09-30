#include "GPIO.h"
#include "timer.h"

void main(){
	LEDS_INIT();
	TIM_INIT();
	while(1)
	{
		LEDS_CONTROL(LED_BLUE, ON_LED);
		LEDS_CONTROL(LED_RED, OFF_LED);
		my_Delay(1000);
		LEDS_CONTROL(LED_BLUE, OFF_LED);
		LEDS_CONTROL(LED_RED, ON_LED);
		my_Delay(1000);
	}
}
void SystemInit(){
	
}
