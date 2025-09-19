///* ---------------- Code 2 */
//
///* */
///* i3g4250d_exti_pe1_baremetal.c
//   For STM32F411, SPI1: PA5/PA6/PA7, CS = PE3, DRDY = PE1 (EXTI1)
//   Build with -O0 / -Og for debugging (Live Expressions).
//*/
//
//#include "stm32f4xx.h"
//#include <stdint.h>
//
///* --- I3G4250D registers --- */
//#define I3G_WHO_AM_I     0x0F
//#define I3G_CTRL_REG1    0x20
//#define I3G_CTRL_REG2    0x21
//#define I3G_CTRL_REG3    0x22
//#define I3G_CTRL_REG5    0x24
//#define I3G_STATUS_REG   0x27
//#define I3G_OUT_X_L      0x28
//
///* CS on PE3 */
//#define CS_LOW()  (GPIOE->BSRR = (1U << (3 + 16))) /* reset PE3 */
//#define CS_HIGH() (GPIOE->BSRR = (1U << 3))        /* set PE3 */
//
///* Volatile globals to inspect in debugger */
//volatile int16_t gx = 0, gy = 0, gz = 0;
//volatile uint32_t isr_count = 0;
//
///* --- Small delay --- */
//static void delay_ms(volatile uint32_t ms) {
//    while (ms--) {
//    	// input vào 1ms thì chạy dc 0.75 ms nếu f = 16MHz
//        for (volatile uint32_t i = 0; i < 12000; ++i) __asm__("nop");
//    }
//}
//
///* --- SPI1 (master) init: CPOL=0, CPHA=0, 8-bit, SSM=1 --- */
//static void SPI1_Init(void) {
//    /* Enable clocks: GPIOA, GPIOE, SPI1 */
//    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOEEN;
//    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
//
//    /* PA5=SCK PA6=MISO PA7=MOSI -> AF5 (SPI1) */
//    /* Set MODER to AF mode (10) */
//    GPIOA->MODER &= ~((3U<<10)|(3U<<12)|(3U<<14));
//    GPIOA->MODER |=  ((2U<<10)|(2U<<12)|(2U<<14));
//    /* Set AF5 for PA5,PA6,PA7 */
//    GPIOA->AFR[0] &= ~((0xF<<(4*5)) | (0xF<<(4*6)) | (0xF<<(4*7)));
//    GPIOA->AFR[0] |=  ((5U<<(4*5)) | (5U<<(4*6)) | (5U<<(4*7)));
//
//    /* Configure PE3 (CS) as push-pull output, initially high */
//    GPIOE->MODER &= ~(3U << (3*2));
//    GPIOE->MODER |=  (1U << (3*2)); // output
//    GPIOE->OTYPER &= ~(1U << 3);    // push-pull
//    GPIOE->OSPEEDR |= (3U << (3*2)); // high speed
//    CS_HIGH();
//
//    /* Configure PE1 as input (DRDY) - keep default input */
//    GPIOE->MODER &= ~(3U << (1*2));
//    GPIOE->PUPDR &= ~(3U << (1*2)); // no pull
//
//    /* SPI1 CR1: Master, SSM=1, SSI=1, BR = fPCLK/16 (BR = 0b010) */
//    SPI1->CR1 = 0;
//    SPI1->CR1 |= SPI_CR1_MSTR;       /* Master */
//    SPI1->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI; /* Software slave management, keep internal NSS high */
//    /* BR bits: 010 => /8 ; use /8 or /16 depending PCLK - safe low speed */
//    /* Use BR = 3 (011) => fPCLK/16 only if macro present. Use SPI_CR1_BR_1 to set 010 (Div8). */
//    SPI1->CR1 |= (0x3 << SPI_CR1_BR_Pos); /* BR = 011 (div 16) */
//    /* CPOL=0, CPHA=0 (default bits cleared) */
//    /* DFF = 8-bit (default) */
//    SPI1->CR1 |= SPI_CR1_SPE; /* enable SPI */
//}
//
///* SPI byte transfer (8-bit) */
//static uint8_t SPI1_Transfer(uint8_t tx) {
//    /* Wait TXE then write */
//    while (!(SPI1->SR & SPI_SR_TXE)) {}
//    *((__IO uint8_t *)&SPI1->DR) = tx; /* write byte (8-bit access) */
//    /* Wait RXNE then read */
//    while (!(SPI1->SR & SPI_SR_RXNE)) {}
//    return *((__IO uint8_t *)&SPI1->DR);
//}
//
///* Write single register (write: bit7=0) */
//static void I3G_WriteReg(uint8_t reg, uint8_t val) {
//    CS_LOW();
//    SPI1_Transfer(reg & 0x7F); /* write command */
//    SPI1_Transfer(val);
//    CS_HIGH();
//}
//
///* Read single register */
//static uint8_t I3G_ReadReg(uint8_t reg) {
//    uint8_t val;
//    CS_LOW();
//    SPI1_Transfer(0x80 | (reg & 0x7F)); /* read command, MSB=1 */
//    val = SPI1_Transfer(0x00);
//    CS_HIGH();
//    return val;
//}
//
///* Burst read 6 bytes OUT_X_L..OUT_Z_H using Read + Auto-increment bits:
//   read bit = 1 (bit7), auto-inc = 1 (bit6) => command = 0xC0 | reg
//*/
//static void I3G_ReadXYZ_burst(int16_t *x, int16_t *y, int16_t *z) {
//    uint8_t buf[6];
//    CS_LOW();
//    SPI1_Transfer(0xC0 | I3G_OUT_X_L);
//    for (int i = 0; i < 6; ++i) buf[i] = SPI1_Transfer(0x00);
//    CS_HIGH();
//
//    *x = (int16_t)((buf[1] << 8) | buf[0]);
//    *y = (int16_t)((buf[3] << 8) | buf[2]);
//    *z = (int16_t)((buf[5] << 8) | buf[4]);
//}
//
///* EXTI1 init (PE1 -> EXTI1) */
//static void EXTI1_Init(void) {
//    /* Enable SYSCFG clock */
//    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
//
//    /* Map EXTI1 to PE1: EXTICR[0] bits 7:4 = port E (0100b = 4) */
//    uint32_t tmp = SYSCFG->EXTICR[0];
//    tmp &= ~(0xF << 4);
//    tmp |= (0x4 << 4);
//    SYSCFG->EXTICR[0] = tmp;
//
//    /* Clear pending and configure edge */
//    EXTI->PR = (1U << 1);
//    EXTI->FTSR &= ~(1U << 1);
//    EXTI->RTSR |=  (1U << 1); /* rising edge */
//    EXTI->IMR  |=  (1U << 1); /* unmask */
//
//    /* NVIC enable EXTI1_IRQn */
//    NVIC_ClearPendingIRQ(EXTI1_IRQn);
//    NVIC_SetPriority(EXTI1_IRQn, 5);
//    NVIC_EnableIRQ(EXTI1_IRQn);
//}
//
///* ISR */
//void EXTI1_IRQHandler(void) {
//    if (EXTI->PR & (1U << 1)) {
//        EXTI->PR = (1U << 1); /* clear pending */
//
//        /* read gyro (burst) */
//        int16_t tx, ty, tz;
//        I3G_ReadXYZ_burst(&tx, &ty, &tz);
//
//        /* store to volatile globals (visible to Live Expressions) */
//        gx = tx;
//        gy = ty;
//        gz = tz;
//        isr_count++;
//    }
//}
///* Helper: read STATUS_REG to see ZYXDA (bit3) */
//static int I3G_DataReady_poll(void) {
//    uint8_t st = I3G_ReadReg(I3G_STATUS_REG);
//    return (st & (1U << 3)) ? 1 : 0;
//}
//
///* Initialize sensor: verify WHO_AM_I, set CTRL_REG1 and CTRL_REG3 */
//static int I3G_InitSensor(void) {
//    uint8_t who = I3G_ReadReg(I3G_WHO_AM_I);
//    if (who != 0xD3) { /* expected WHO_AM_I for I3G4250D */
//        return -1;
//    }
//
//    /* CTRL_REG1: PD=1, enable X/Y/Z; choose ODR/BW as needed
//       Example: 0x0F -> PD=1, enable all axes, BW/ODR default low */
//    I3G_WriteReg(I3G_CTRL_REG1, 0x0F);
//    delay_ms(10);
//
//    /* CTRL_REG3: I2_DRDY = 1 (bit3), H_Lactive=0, PP_OD=0 => value 0x08 */
//    I3G_WriteReg(I3G_CTRL_REG3, 0x08);
//    delay_ms(5);
//
//    /* Optionally: CTRL_REG5 left default. */
//    /* read-back verify */
//    uint8_t c1 = I3G_ReadReg(I3G_CTRL_REG1);
//    uint8_t c3 = I3G_ReadReg(I3G_CTRL_REG3);
//
//
//    (void)c1; (void)c3; /* keep for debug watch (or use them to decide) */
//    return 0;
//}
//
///* MAIN */
//int main(void) {
//    /* Basic clock configuration assumed done by system/startup */
//    /* Init SPI1 + CS + EXTI */
//    SPI1_Init();
//    EXTI1_Init();
//
//    /* Basic test: software-trigger EXTI to ensure ISR mechanism works */
//
//    isr_count = 0;
//    gx = gy = gz = 0;
//    //EXTI->SWIER |= (1U << 1); /* software trigger EXTI1 */
//    delay_ms(10);
//    /* After this you should see isr_count==1 and gx/gy/gz updated (but may be zero if sensor not yet) */
//
//    /* Initialize sensor and configure DRDY */
//    if (I3G_InitSensor() != 0) {
//        /* WHO_AM_I failed - handle error (spin) */
//        while (1) { __asm__("nop"); }
//    }
//    /* Optional: check Data Ready by polling STATUS_REG for a few loops
//       to confirm sensor producing data (useful for debugging before relying on DRDY) */
//    for (int i = 0; i < 1000; ++i) {
//        if (I3G_DataReady_poll()) {
//            /* read once to clear */
//            int16_t tx, ty, tz;
//            I3G_ReadXYZ_burst(&tx, &ty, &tz);
//            (void)tx; (void)ty; (void)tz;
//            break; // thoát khỏi vòng lặp for ngay
//        }
//        delay_ms(1);
//    }
//
//    /* Now main loop - ISR will update gx/gy/gz when DRDY toggles */
//    while (1) {
//        /* You can add a small task here; keep CPU free for interrupts */
//    	//I3G_ReadXYZ_burst(&gx, &gy, &gz);
//        // __asm__("wfi"); /* wait for interrupt - saves power */
//    }
//}

#include "main.h"
#include "stm32f4xx.h"

/* ------------------------ UART */
#define GPIOB_BASE_ADDR 0x40020400
#define UART1_BASE_ADDR 0x40011000

void UART1_INIT(){
	__HAL_RCC_GPIOB_CLK_ENABLE();
	uint32_t *GPIOB_MODER = (uint32_t *)(GPIOB_BASE_ADDR + 0x00);
	uint32_t *GPIOB_AFRL = (uint32_t *)(GPIOB_BASE_ADDR + 0x20);
	*GPIOB_MODER &= ~(0b1111 << 12);
	*GPIOB_MODER |= (0b1010 << 12);

	*GPIOB_AFRL &= ~(0xFF << 24);
	*GPIOB_AFRL |= (0x77 << 24);

	// SET UP UART1
	__HAL_RCC_USART1_CLK_ENABLE();
	uint32_t *UART1_CR1 = (uint32_t *)(UART1_BASE_ADDR + 0x0C);
	uint32_t *UART1_BRR = (uint32_t *)(UART1_BASE_ADDR + 0x08);

	*UART1_CR1 |= (0b1 << 12); // Word length: 9 bits ( + 1 Parity)
	*UART1_CR1 |= (0b1 << 10); // Parity: Enable
	*UART1_CR1 &= ~(0b1 << 9); // Parity type: even
	*UART1_CR1 |= (0b11 << 2); // Transmit & Receiver enable
	*UART1_CR1 |= (0b1 << 13); // Enable UART

	*UART1_BRR = (104 << 4) | (3 << 0) ;// Set Baudrate 9600 bps

	/* ----------------- UART Interrupt */
	*UART1_CR1 |= (0b1 << 5); // Set bit RXNEIE
	// Position = 37 -> Bit 5 NVIC_ISER1(0xE000E104)

	uint32_t *NVIC_ISER1 = (uint32_t *)(0xE000E104);
	*NVIC_ISER1 |= (0b1 << 5);

}
void UART_SEND(uint8_t data){
	uint32_t *UART1_DR = (uint32_t *)(UART1_BASE_ADDR + 0x04);
	*UART1_DR = data;

	uint32_t *UART1_SR = (uint32_t *)(UART1_BASE_ADDR + 0x00);
	while(((*UART1_SR >> 6) & 1) == 0);
}

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
void my_printf(char *str,...){
	va_list list;
	va_start(list, str);
	char print_buf [128] = {0};
	vsprintf(print_buf, str, list);
	int len = strlen(print_buf);
	for (int i = 0; i < len; i++){
		UART_SEND(print_buf[i]);
	}
	va_end(list);
}

/* --------------------- Config CS --------------*/
#define GPIOE_BASE_ADDR 0x40021000
void CS_LOW(){
	uint32_t *GPIOE_ODR = (uint32_t *)(GPIOE_BASE_ADDR + 0x14);
	*GPIOE_ODR &= ~(1 << 3);
}
void CS_HIGH(){
	uint32_t *GPIOE_ODR = (uint32_t *)(GPIOE_BASE_ADDR + 0x14);
	*GPIOE_ODR |= 1 << 3;
}

/* --------------------- SPI_INIT ---------------*/
#define GPIOA_BASE_ADDR 0x40020000
#define SPI1_BASE_ADDR 0x40013000

volatile int16_t gx_dps=0, gy_dps=0, gz_dps=0; // Biến lưu giá trị Dps <- Raw
volatile int16_t gx = 0 , gy = 0 , gz = 0; // gx,gy,gz có thể  < 0 => int16_t
volatile int16_t gyro_buf[32][3];   // lưu tối đa 32 bộ X,Y,Z

void SPI_INIT(){
	// PA5, PA6, PA7: Alternate function mode
	__HAL_RCC_GPIOA_CLK_ENABLE();
	uint32_t *GPIOA_MODER = (uint32_t *)(GPIOA_BASE_ADDR + 0x00);
	*GPIOA_MODER &= ~(0b111111 << 10);
	*GPIOA_MODER |= (0b101010 << 10);

	// PA5 : SPI1_SCK, PA6 : SPI1_MISO, PA7 : SPI1_MOSI -> vào AF5: 0101
	uint32_t *GPIOA_AFRL = (uint32_t *)(GPIOA_BASE_ADDR + 0x20);
	*GPIOA_AFRL &= ~(0xFFF << 20);
	*GPIOA_AFRL |= 0x555 << 20;

	// PE3: CS -> General purpose output mode
	__HAL_RCC_GPIOE_CLK_ENABLE();
	uint32_t *GPIOE_MODER = (uint32_t *)(GPIOE_BASE_ADDR + 0x00);
	*GPIOE_MODER &= ~(0b11 << 6);
	*GPIOE_MODER |= (0b01 << 6);
	CS_HIGH(); // Important

	// Config SPI control_register
	__HAL_RCC_SPI1_CLK_ENABLE();
	uint16_t *SPI1_CR1 = (uint16_t *)(SPI1_BASE_ADDR + 0x00);
	*SPI1_CR1 |= (0b1 << 2); // Master mode
	*SPI1_CR1 |= (0b011 << 3); // Set Clock f/16 = 16MHz / 16 = 1MHz <= 10 MHz (oke)
	*SPI1_CR1 |= (0b11 << 8); // Software slave management enable
	*SPI1_CR1 |= (1 << 1);  // CPOL = 1
	*SPI1_CR1 |= (1 << 0);  // CPHA = 1
	*SPI1_CR1 |= (1 << 6); // Enable SPI

//	while (((*(volatile uint16_t *)(SPI1_BASE_ADDR+0x08)) & (1<<0))) {
//	    (void)(*(volatile uint8_t *)(SPI1_BASE_ADDR+0x0C)); // đọc DR nếu RXNE=1
//	}
}

uint8_t SPI_Transfer(uint8_t data)
{
	volatile uint8_t *SPI1_DR = (uint8_t *)(SPI1_BASE_ADDR + 0x0C);
	volatile uint16_t *SPI1_SR = (uint16_t *)(SPI1_BASE_ADDR + 0x08);

	while (((*SPI1_SR >> 1) & 1) == 0); // bit 1:TXE = 0 -> wait
	*SPI1_DR = data;

	while (((*SPI1_SR >> 7) & 1) == 1); // Busy bit
	while (((*SPI1_SR >> 0) & 1) == 0); // bit 0: RXNE = 0 -> wait

	while (((*SPI1_SR >> 7) & 1) == 1); // Busy bit
	uint8_t rb = (*SPI1_DR);

	return rb;
}

void SPI_Write_I3G4250D(uint8_t reg, uint8_t data)
{
	CS_LOW();
	(void)SPI_Transfer(reg & 0x7F); // Write Mode
	(void)SPI_Transfer(data);
	CS_HIGH();
}

uint8_t SPI_Read_I3G4250D(uint8_t reg)
{
	CS_LOW();
	(void)SPI_Transfer(reg | (0b1 << 7)); // Read Mode
	uint8_t val = SPI_Transfer(0x00); // 0x00 : dummy data kích clock
	CS_HIGH();

	return val;
}

void SPI1_Read_multiByte(uint8_t reg, uint8_t *buf, uint8_t len)
{
	CS_LOW();
	SPI_Transfer(reg | (0b11 << 6)); // Read Mode and auto increment
	for (uint8_t i = 0 ; i < len; i++){
		buf[i] = SPI_Transfer(0x00);
	}
	CS_HIGH();
}

void I3G4250D_ReadXYZ(volatile int16_t* x, volatile int16_t* y, volatile int16_t* z){
    uint8_t buffer[6];
    CS_LOW();
    SPI_Transfer(0x80 | 0x40 | 0x28);
    for(int i=0;i<6;i++)
    	buffer[i] = SPI_Transfer(0x00);
    CS_HIGH();

    *x = (int16_t)(buffer[1]<<8 | buffer[0]);
    *y = (int16_t)(buffer[3]<<8 | buffer[2]);
    *z = (int16_t)(buffer[5]<<8 | buffer[4]);
}
void RawToDps (volatile int16_t* x, volatile int16_t* y, volatile int16_t* z){

	gx_dps = (*x) * 8.75/1000;
	gy_dps = (*y) * 8.75/1000;
	gz_dps = (*z) * 8.75/1000;
}

/* ----------------------- PE1 <-> DRDY/INT2 of sensor I3G4250D ------------*/
#define EXTI_BASE_ADDR 0x40013C00
#define SYSCFG_BASE_ADDR 0x40013800
#define Rising_trigger 0x08
#define Falling_trigger 0x0C
#define Interrupt_mask_register 0x00
#define Pending_Register 0x14 // clear interrupt

volatile uint16_t tmp2 = 0;
volatile uint16_t test = 0; // Khởi tạo biến test để test nhiều function
// test = 1 : Ngắt cho DRDY -> Data ready thì ngắt.
// test = 2 : Ngắt cho Watermark của bộ nhớ buffer FIFO.

void EXTI1_Init(){
	// Map PE1 -> EXTI1
	__HAL_RCC_SYSCFG_CLK_ENABLE();
	uint32_t *SYSCFG_EXTICR1 = (uint32_t *)(SYSCFG_BASE_ADDR + 0x08);
	*SYSCFG_EXTICR1 &= ~(0xF << 4);
	*SYSCFG_EXTICR1 |= (0x4 << 4);

	// Set thanh ghi rising
	uint32_t *EXTI_RTSR = (uint32_t *)(EXTI_BASE_ADDR + Rising_trigger);
	*EXTI_RTSR |= (0b1 << 1);

	// Set PE1 : GPIOE_INPUT
	uint32_t *GPIOE_MODER = (uint32_t *)(GPIOE_BASE_ADDR + 0x00);
	*GPIOE_MODER &= ~(0b11 << 2);

	// Set interrupt mask
	uint32_t *EXTI_IMR = (uint32_t *)(EXTI_BASE_ADDR + Interrupt_mask_register);
	*EXTI_IMR |= (0b1 << 1); // EXTI line 1

	// Set bit so 7 cua thanh ghi NVIC_ISER0 -> de bat EXTI1 co position = 1
	uint32_t *EXTI_ISER0 = (uint32_t *)0xE000E100;
	*EXTI_ISER0 |= (0b1 << 7);

}

void EXTI1_IRQHandler()
{
	if (test == 1)      // Test Bypass -> INT DRDY
	{// Test cho DRDY
		uint32_t *EXTI_PR = (uint32_t *)(EXTI_BASE_ADDR + Pending_Register);
		// Clear pending ngay lập tức để tránh retrigger spam (viết 1 vào bit)
		*EXTI_PR |= (1 << 1);
		I3G4250D_ReadXYZ(&gx, &gy, &gz);
	}
	else if (test == 2)  // Test FIFO Stream Mode -> INT Watermark
	{
		uint32_t *EXTI_PR = (uint32_t *)(EXTI_BASE_ADDR + Pending_Register);
		// Clear pending ngay lập tức để tránh retrigger spam (viết 1 vào bit)
		*EXTI_PR |= (1 << 1);

		// Đọc giá trị của thanh ghi FIFO_SRC_REG (0x2F)
		uint8_t tmp;
		tmp = SPI_Read_I3G4250D(0x2F);

		uint8_t FSS = tmp & (0x1F);
		uint8_t OVRN = (tmp >> 6) & (0x01);
		if (FSS == 0) return ;
		/* ----- synchronous FIFO read (OVRN == 0) -----*/
		if (OVRN == 0){
			for (uint8_t i = 0; i < FSS ; i++){
				volatile int16_t tx , ty, tz;
				I3G4250D_ReadXYZ(&tx, &ty, &tz);
			}
		}

	}
}

static int I3G_DataReady_poll()
{
	uint8_t st = SPI_Read_I3G4250D(0x27);

	return ((st >> 3) & (0b1)) ? 1 : 0 ;
}

volatile uint8_t checkinit = 0;

/* ------------------  I3G_FIFO_Enable_Stream_With_Watermark -------- */
#define I3G_CTRL_REG3 0x22
#define I3G_CTRL_REG5 0x24
#define I3G_FIFO_CTRL 0x2E

void I3G4250D_FIFO_Enable_Stream_With_Watermark(uint8_t watermark)
{
	// Muốn bật bit nào thì cần đọc lại cả thanh ghi rồi | bit đó.
	uint8_t tmp ;
	tmp = SPI_Read_I3G4250D(I3G_CTRL_REG5);
	tmp |= 0x40;
	SPI_Write_I3G4250D(I3G_CTRL_REG5, tmp); // On bit 6: 0100 0000

	uint8_t fm_stream = (0x02 << 5) ;  // Set FIFO Mode: Stream (0b010)
	uint8_t wtm = (watermark & 0x1F);
	// Thanh ghi này không cần đọc vì cả thanh ghi đều là giá trị cần set.
	SPI_Write_I3G4250D(I3G_FIFO_CTRL, fm_stream | wtm);

	// Set bit I2_WTM
	tmp = SPI_Read_I3G4250D(I3G_CTRL_REG3);
	tmp |= (1U << 2);
	SPI_Write_I3G4250D(I3G_CTRL_REG3, tmp); // On bit 6: 0100 0000
}

void I3G4250D_Init()
{
	SPI_Write_I3G4250D(0x20, 0x0F) ; // ODR : 100 (Hz)

	test = 2;
	if(test == 1){
		SPI_Write_I3G4250D(0x22, 0x08) ; // DYDR/INT2
	}else if (test == 2)
	{
		I3G4250D_FIFO_Enable_Stream_With_Watermark(16);
	}
}
int main(){
	HAL_Init();
	//UART1_INIT();
	SPI_INIT();
	I3G4250D_Init();

	// --- IMPORTANT: nếu DRDY đã HIGH (sensor trả dữ liệu đầu), đọc 1 lần để clear nó ---
	if (I3G_DataReady_poll()) { // ZYXDA = bit3
		int16_t tx, ty, tz;
		I3G4250D_ReadXYZ(&tx, &ty, &tz); // đọc 1 lần để clear DRDY
		__asm__("nop"); // debug doc gia tri tx,ty,tz
	}
	EXTI1_Init();

	while(1){
		RawToDps(&gx, &gy, &gz);
		//my_printf("Gia tri cua thanh Who_Am_I = %d \n",tmp);
	}
	return 0;
}




