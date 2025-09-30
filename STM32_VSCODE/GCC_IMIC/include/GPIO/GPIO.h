#ifndef _GPIO_H
#define _GPIO_H
typedef enum{
	LED_GREEN,
	LED_ORANGE,
	LED_RED,
	LED_BLUE
}led_t;

typedef enum{
	OFF_LED,
	ON_LED
}led_state_t;
void LEDS_INIT();
void LEDS_CONTROL(led_t led, led_state_t state);

#endif 