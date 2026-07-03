#include "main.h"
#include "stm32f4xx.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/* -------------------- 1. UART --------------------- */
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

//  Hàm gửi chuỗi kèm giá trị biến lên UART
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

/* --------------------- 2. SPI_INIT ---------------*/
#define GPIOA_BASE_ADDR 0x40020000 // PA5 : SPI1_SCK, PA6 : SPI1_MISO, PA7 : SPI1_MOSI
#define SPI1_BASE_ADDR 0x40013000  // Dùng cho Config SPI
#define GPIOE_BASE_ADDR 0x40021000 // Dùng cho PE1

#define I3G_CTRL_REG3 	   0x22
#define I3G_CTRL_REG5 	   0x24
#define I3G_FIFO_CTRL      0x2E
#define I3G_CTRL_REG1      0x20
#define I3G_CTRL_REG2      0x21
#define I3G_CTRL_REG3      0x22
#define I3G_CTRL_REG5      0x24
#define I3G_REFERENCE      0x25
#define I3G_INT1_THS_XH    0x32
#define I3G_INT1_THS_XL    0x33
#define I3G_INT1_THS_YH    0x34
#define I3G_INT1_THS_YL    0x35
#define I3G_INT1_THS_ZH    0x36
#define I3G_INT1_THS_ZL    0x37
#define I3G_INT1_DURATION  0x38
#define I3G_INT1_CFG       0x30
#define I3G_INT1_SRC       0x31

// ----- Các global variable
volatile int16_t gx_dps=0, gy_dps=0, gz_dps=0; // Biến lưu giá trị Dps <- Raw
volatile int16_t gx = 0 , gy = 0 , gz = 0; // gx,gy,gz có thể  < 0 => int16_t

volatile int16_t gyro_buf[32][3];   // lưu tối đa 32 bộ X,Y,Z
volatile int16_t gyro_FIFO_dps[32][3];   // lưu tối đa 32 bộ X,Y,Z
volatile uint8_t sample_count = 0;

volatile uint16_t test = 1; // Khởi tạo biến test để test nhiều function
// test = 1 : Ngắt cho DRDY -> Data ready thì ngắt.
// test = 2 : Ngắt cho Watermark của bộ nhớ buffer FIFO.

volatile uint16_t checkInter = 0;
// ----- Config CS
void CS_LOW(){
	uint32_t *GPIOE_ODR = (uint32_t *)(GPIOE_BASE_ADDR + 0x14);
	*GPIOE_ODR &= ~(1 << 3);
}

void CS_HIGH(){
	uint32_t *GPIOE_ODR = (uint32_t *)(GPIOE_BASE_ADDR + 0x14);
	*GPIOE_ODR |= 1 << 3;
}

// ----- CONFIG SPI
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

// ----- SPI Function
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

void Raw_to_DPS_FIFO(volatile int16_t *a, uint8_t m, uint8_t n){
	for (uint8_t i = 0 ; i < m; i ++){
		for (uint8_t j = 0 ; j < n ; j ++){
			gyro_FIFO_dps[i][j] = *(a + n*i + j) * 8.75/1000;
		}
	}
}

static int I3G_DataReady_poll()
{
	uint8_t st = SPI_Read_I3G4250D(0x27);

	return ((st >> 3) & (0b1)) ? 1 : 0 ;
}

// ------ FIFO_Stream_With_Watermark
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
	tmp |= (0b1 << 2);
	SPI_Write_I3G4250D(I3G_CTRL_REG3, 0b1 << 2); // On bit 2: 0000 0100
}

// ------ HPF / INT1_THS / INT1_DURATION
void I3G_Enable_HPF()
{
	// HPM: Normal , HPCF = 8 (Hz)
	SPI_Write_I3G4250D(I3G_CTRL_REG2, 0x00);

	// CTRL_REG5: High-pass-filtered data are used for interrupt generation
	SPI_Write_I3G4250D(I3G_CTRL_REG5, 0x05);

	// Dummy read of REFERENCE to reset HP filter reference (optional, recommended)
	(void)SPI_Read_I3G4250D(I3G_REFERENCE);
}
void I3G_Enable_THUS_DUR(float threshold_dps, uint8_t duration_counts, uint8_t and_or_lir_bits)
{
	/*  1. Max threshold_dps = 32767 (0x7FFF) -> same for all axes.
		2. Duration_counts (max = 127 sample): Số mẫu lớn hơn thereshold để interrupt
		     -> Thời gian có mẫu lớn hơn liên tục: t = D * (1/ODR))
		3. and_or_lir_bits:
			|_> [1] : And -> tất cả các axis vượt thì mới interrupt.
			|_> [0] : OR -> một axis vượt là vào interrupt.
		4. Formula: threshold raw = (threshold_dps / 0.00875 )
	*/

	// Convert dps -> raw
	uint32_t raw = (uint32_t)((threshold_dps / 0.00875f) + 0.5f); // +0.5f để làm tròn lên
	uint8_t th_low = raw & (0xFF) ;
	uint8_t th_hig = (raw >> 8) & (0x7F);

	// Write X, Y, Z threshold
	SPI_Write_I3G4250D(I3G_INT1_THS_XH, th_hig);
	SPI_Write_I3G4250D(I3G_INT1_THS_XL, th_low);

	SPI_Write_I3G4250D(I3G_INT1_THS_YH, th_hig);
	SPI_Write_I3G4250D(I3G_INT1_THS_YL, th_low);

	SPI_Write_I3G4250D(I3G_INT1_THS_ZH, th_hig);
	SPI_Write_I3G4250D(I3G_INT1_THS_ZL, th_low);

	// Duration: Wait = 0 , Duration (N <= 127);
	SPI_Write_I3G4250D(I3G_INT1_DURATION, (duration_counts & 0x7F) | (0b0 << 7)); // Wait = 0

	// INT1_CFG : ON ZHIE -> XLIE
	uint8_t temp = (0b1 << 0) | (and_or_lir_bits << 6);
	SPI_Write_I3G4250D(I3G_INT1_CFG, temp);

	// CRTL_REG3 : INT1 — set polarity as needed (H_Lactive bit)
	SPI_Write_I3G4250D(I3G_CTRL_REG3, 0x88);
}

// ------ I3G4250D_Init
void I3G4250D_Init()
{
	// Phải bật Sensor sau cùng -> bởi vì tốc độ đọc 32 slot ở FIFO rất nhanh
	// đến mức chưa kịp set Watermark
	SPI_Write_I3G4250D(0x20, 0x0F) ; // ODR = 100 (Hz)

	if(test == 1){
		SPI_Write_I3G4250D(0x22, 0x08) ; // DYDR/INT2
		// --- IMPORTANT: nếu DRDY đã HIGH (sensor trả dữ liệu đầu), đọc 1 lần để clear nó ---
		if (I3G_DataReady_poll()) { // ZYXDA = bit3
			int16_t tx, ty, tz;
			I3G4250D_ReadXYZ(&tx, &ty, &tz); // đọc 1 lần để clear DRDY
			__asm__("nop"); // debug doc gia tri tx,ty,tz
		}
		//I3G_Enable_HPF();
		I3G_Enable_THUS_DUR(100, 2, 0b00); // Threshold = 100dps , OR

	}else if (test == 2)
	{
		// Trước khi chạy cần chuyển sang ByPass để clear toàn bộ FIFO rác ở những lần chạy trước
		SPI_Write_I3G4250D(I3G_FIFO_CTRL, 0x00);
		I3G4250D_FIFO_Enable_Stream_With_Watermark(16);
	}
}

/* ----------------------- 3. EXTI PE1 <-> DRDY/INT2 of sensor I3G4250D ------------*/
#define EXTI_BASE_ADDR 0x40013C00

#define SYSCFG_BASE_ADDR 0x40013800
#define Rising_trigger 0x08
#define Falling_trigger 0x0C
#define Interrupt_mask_register 0x00
#define Pending_Register 0x14 // clear interrupt

void EXTI0_Init(){
	// Map PE[x] -> EXTI0
	__HAL_RCC_SYSCFG_CLK_ENABLE();
	uint32_t *SYSCFG_EXTICR1 = (uint32_t *)(SYSCFG_BASE_ADDR + 0x08);
	*SYSCFG_EXTICR1 &= ~(0xF << 0);
	*SYSCFG_EXTICR1 |= (0x4 << 0);

	// Set thanh ghi rising
	uint32_t *EXTI_RTSR = (uint32_t *)(EXTI_BASE_ADDR + Rising_trigger);
	*EXTI_RTSR |= (0b1 << 0);

	// Set PE0 : GPIOE_INPUT
	uint32_t *GPIOE_MODER = (uint32_t *)(GPIOE_BASE_ADDR + 0x00);
	*GPIOE_MODER &= ~(0b11 << 0);

	// Set interrupt mask
	uint32_t *EXTI_IMR = (uint32_t *)(EXTI_BASE_ADDR + Interrupt_mask_register);
	*EXTI_IMR |= (0b1 << 0); // EXTI line 0

	// Set bit so 6 cua thanh ghi NVIC_ISER0 -> de bat EXTI0 co position = 0
	uint32_t *EXTI_ISER0 = (uint32_t *)0xE000E100;
	*EXTI_ISER0 |= (0b1 << 6);

}
void EXTI1_Init()
{
	// Map PE[x] -> EXTI1
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

void EXTI0_IRQHandler()
{
	// Dùng nếu dùng Latch
//	 volatile uint8_t src = SPI_Read_I3G4250D(0x31); // đọc để clear IA & kéo INT1 về idle
//	 (void)src;

	uint32_t *EXTI_PR = (uint32_t *)(EXTI_BASE_ADDR + Pending_Register);
	// Clear pending ngay lập tức để tránh retrigger spam (viết 1 vào bit)
	*EXTI_PR |= (1 << 0);

	checkInter ++;
}

void EXTI1_IRQHandler()
{
	if (test == 1)      // Test Bypass MODE -> trigger INT DRDY
	{
		uint32_t *EXTI_PR = (uint32_t *)(EXTI_BASE_ADDR + Pending_Register);
		// Clear pending ngay lập tức để tránh retrigger spam (viết 1 vào bit)
		*EXTI_PR |= (1 << 1);
		__asm__("nop");
		I3G4250D_ReadXYZ(&gx, &gy, &gz);
	}
	else if(test == 2)  // Test FIFO Stream Mode -> INT Watermark
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
		/* ----- FIFO read  -----*/
		uint8_t nread = (FSS < 31) ? FSS : (FSS + (OVRN ? 1 : 0));
		for (uint8_t i = 0; i < nread ; i++){
			volatile int16_t tx , ty, tz;
			I3G4250D_ReadXYZ(&tx, &ty, &tz);
			gyro_buf[sample_count][0] = tx;
			gyro_buf[sample_count][1] = ty;
			gyro_buf[sample_count][2] = tz;
			sample_count = (sample_count + 1) % 32;
		}
	}
}

volatile uint8_t checkinit = 0;
volatile uint8_t checkinit2 = 0;
int main(){
	HAL_Init();
	//UART1_INIT();
	SPI_INIT();

	EXTI1_Init();
	EXTI0_Init();

	I3G4250D_Init();
	checkinit = SPI_Read_I3G4250D(0x30);
	while(1){
		if (test == 1)
		{
			RawToDps(&gx, &gy, &gz);

		}else if (test == 2)
		{
			Raw_to_DPS_FIFO((volatile int16_t *)gyro_buf, 32, 3);
		}

		//my_printf("Gia tri cua thanh Who_Am_I = %d \n",tmp);
	}
	return 0;
}
