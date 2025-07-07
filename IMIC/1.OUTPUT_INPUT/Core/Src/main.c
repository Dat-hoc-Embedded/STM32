# include <main.h>

/*------------------- LEDs */
// LED on board is: PD12, PD13, PD14, PD15 đang ở trạng thái nối GND
#define GPIOD_BASE_ADDR 0x40020C00 // BASE ADDRESS GPIOD

// Function MODER INIT for LEDs
void LEDS_INIT(){
	// Khoi tao CLOCK cho GPIOD
	__HAL_RCC_GPIOD_CLK_ENABLE();

	uint32_t* GPIOD_MODER = GPIOD_BASE_ADDR + 0x00;
	// Reset bit MODER of PD 12,13,14,15
	*GPIOD_MODER &= ~(0xFF << 24); // 0xFF = 0x1111 1111

	// SET [01] cho MODE: OUTPUT
	*GPIOD_MODER |= (0b01 << 24); // LED GREEN bits [24:25]
	*GPIOD_MODER |= (0b01 << 26); // LED ORANGE ...
	*GPIOD_MODER |= (0b01 << 28); // LED RED ...
	*GPIOD_MODER |= (0b01 << 30); // LED BLUE ...
}

// Function for Control Leds
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

void LEDS_CONTROL(led_t led, led_state_t state){
	uint32_t *GPIOD_ODR = GPIOD_BASE_ADDR + 0x14;
	if (state == ON_LED)
		*GPIOD_ODR |= (0b1 << (12 + led));
	else
		*GPIOD_ODR &= ~(0b1 << (12 + led));
}
// FUNCTION OPTIONAL
void BLINK_FULL_LEDS(){
	LEDS_CONTROL(LED_GREEN, ON_LED);
	LEDS_CONTROL(LED_ORANGE, ON_LED);
	LEDS_CONTROL(LED_RED, ON_LED);
	LEDS_CONTROL(LED_BLUE, ON_LED);
	HAL_Delay(200);

	LEDS_CONTROL(LED_GREEN, OFF_LED);
	LEDS_CONTROL(LED_ORANGE, OFF_LED);
	LEDS_CONTROL(LED_RED, OFF_LED);
	LEDS_CONTROL(LED_BLUE, OFF_LED);
	HAL_Delay(200);
}

/*---------------------- BUTTON  */
// USER BUTTON: PA0
#define GPIOA_BASE_ADDR 0x40020000

void BUTTON_INIT(){
	__HAL_RCC_GPIOA_CLK_ENABLE();
	uint32_t* GPIOA_MODER = GPIOA_BASE_ADDR + 0x00;
	// Reset [0:1] -> PA0 : INPUT
	*GPIOA_MODER &= ~(0b11 << 0);

	uint32_t* GPIOA_PUPDR = GPIOA_BASE_ADDR + 0x0C;
	// Set No Pull-up,No Pull-down cho PA0
	*GPIOA_PUPDR &= ~(0b11 << 0);
}
// FUNCTION READ SIGNAL pin PA0
char BUTTON_GET_STATE(){
	uint32_t *GPIOA_IDR = GPIOA_BASE_ADDR + 0x10;
	if (((*GPIOA_IDR >> 0 ) & 0b1) == 1)
		return 1;
	else
		return 0;
}
// int cnt;
int state_green = OFF_LED;
int state_red = OFF_LED;
int main()
{
	HAL_Init();
	LEDS_INIT();
	BUTTON_INIT();
	while(1){
		/* --------- Test OUTPUT */
//		for(int i = 0; i < 4; i++){
//			LEDS_CONTROL(i, ON_LED);
//			HAL_Delay(200);
//			LEDS_CONTROL(i, OFF_LED);
//			HAL_Delay(200);
//
//		}
//		for(int i = 0; i < 4; i++){
//			BLINK_FULL_LEDS();
//		}
		/* --------- Test INPUT */
//		if (BUTTON_GET_STATE()==1)
//		{
//			cnt ++;
//			HAL_Delay(300); // bỏ qua nhiễu của nút nhấn
//			while(BUTTON_GET_STATE()==1); // chống dội phím
//		}
//		if(cnt%2==0)
//		{
//			LEDS_CONTROL(LED_RED, ON_LED);
//		}else{
//			LEDS_CONTROL(LED_RED, OFF_LED);
//		}
		/* BTVN: Nhấn một cái sáng, double tắt, giữ sáng led xanh, giữ lần nữa tắt */

		int identify = 0;  // = 1 là nhấn, = 2 là giữ, = 0 là không nhấn
		if (BUTTON_GET_STATE() == 1){
			identify = 1;
			HAL_Delay(200); // bỏ qua nhiễu của nút nhấn, và đợi 500 mà vẫn có tín hiệu là giữ
			while(BUTTON_GET_STATE()==1) identify = 2; // chống dội phím
		}
		if (identify == 2){
			LEDS_CONTROL(LED_GREEN, !state_green);
			state_green = !state_green ;
		}
		if(identify == 1){
			LEDS_CONTROL(LED_RED, !state_red);
			state_red = !state_red ;
		}
	}
	return 0;
}
