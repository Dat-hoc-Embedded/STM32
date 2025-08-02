#include "main.h"
#include "stm32f4xx.h"

#define GPIOB_BASE_ADDR 0x40020400
#define I2C1_BASE_ADDR  0x40005400
void I2C_INIT(){
	__HAL_RCC_GPIOB_CLK_ENABLE();
	uint32_t *GPIOB_MODER = (uint32_t *)(GPIOB_BASE_ADDR + 0x00);
	/* 1. SET MODER ------------------------- */
	// Reset MODER PIN 6 & 9
	*GPIOB_MODER &= ~(0b11 << 12) ;
	*GPIOB_MODER &= ~(0b11 << 18) ;

	// PIN 6 & PIN 9 -> 10 : Alternate function mode
	*GPIOB_MODER |= ((0b10 << 12) | (0b10 << 18)) ;

	/* 2. Config AFRL & AFRH register ------- */
	// PB6 : AFRL
	uint32_t *GPIOB_AFRL = (uint32_t *)(GPIOB_BASE_ADDR + 0x20);
	*GPIOB_AFRL &= ~(0b1111 << 24);
	*GPIOB_AFRL |= (0b0100 << 24); // AF4

	// PB9 : AFRH
	uint32_t *GPIOB_AFRH = (uint32_t *)(GPIOB_BASE_ADDR + 0x24);
	*GPIOB_AFRH &= ~(0b1111 << 4);
	*GPIOB_AFRH |= (0b0100 << 4); // AF4

	__HAL_RCC_I2C1_CLK_ENABLE();
	/* 3. Config Clock Peripheral và Clock SCL*/
	uint16_t *I2C1_CR1 = (uint16_t *)(I2C1_BASE_ADDR + 0x00);
	uint16_t *I2C1_CR2 = (uint16_t *)(I2C1_BASE_ADDR + 0x04);
	uint16_t *I2C1_CCR = (uint16_t *)(I2C1_BASE_ADDR + 0x1C);

	// Disable I2C
	*I2C1_CR1 &= ~(0b1 << 0);
	*I2C1_CR2 = (16 << 0); // Set Clock Peripheral: 16 MHz
	*I2C1_CCR = (80 << 0); // Set Clock SCL 100kHz
		// Enable I2C
	*I2C1_CR1 |= (0b1 << 0);

}
void I2C_write_CTRL_REG1_A(uint8_t slave_addr, uint8_t reg, uint8_t data){
	uint16_t *I2C1_CR1 = (uint16_t *)(I2C1_BASE_ADDR + 0x00);
	uint16_t *I2C1_DR  = (uint16_t *)(I2C1_BASE_ADDR + 0x10);
	uint16_t *I2C1_SR1 = (uint16_t *)(I2C1_BASE_ADDR + 0x14);
	uint16_t *I2C1_SR2 = (uint16_t *)(I2C1_BASE_ADDR + 0x18);

	// 1. Truyền Start bit
	*I2C1_CR1 |= (1 << 8);
		// Wait until Start condition generated
		while(((*I2C1_SR1 >> 0 ) & 1) == 0);

	// 2. Gửi địa chỉ của Accelerometer
	*I2C1_DR = (slave_addr << 1) | 0 ;
		// Wait until ADDR bit -> set
		while (((*I2C1_SR1 >> 1) & 1) == 0);
		// Clear bit ADDR
		volatile uint8_t tmp;
		tmp = (*I2C1_SR1);
		tmp = (*I2C1_SR2);

	/* 3. Check ACK */
	while (((*I2C1_SR1 >> 10) & 1) == 1); // có lỗi -> wait

	/* 4. Gửi địa chỉ của thanh ghi CTRL_REG1_A (0x20)  */
	*I2C1_DR = reg;
		// wait TXE bit until data is send
		while (((*I2C1_SR1 >> 7) & 1) == 0);

	/* 5. Check ACK */
	while (((*I2C1_SR1 >> 10) & 1) == 1); // có lỗi

	/* 6. Write data to DR_register */
	*I2C1_DR = data; // ~ 21(dec)
		// wait until data is send
		while (((*I2C1_SR1 >> 7) & 1) == 0);

	/* 7. Check ACK */
	while (((*I2C1_SR1 >> 10) & 1) == 1); // có lỗi

	/* 8. Send stop bit CR1:STOP[9] = 1 */
	*I2C1_CR1 |= (1 << 9) ;
}
volatile uint8_t r_data;
uint8_t I2C_read_CTRL_REG1_A(uint8_t slave_addr, uint8_t reg){
	uint8_t data;

	uint16_t *I2C1_CR1 = (uint16_t *)(I2C1_BASE_ADDR + 0x00);
	uint16_t *I2C1_SR1 = (uint16_t *)(I2C1_BASE_ADDR + 0x14);
	uint16_t *I2C1_SR2 = (uint16_t *)(I2C1_BASE_ADDR + 0x18);
	uint16_t *I2C1_DR = (uint16_t *)(I2C1_BASE_ADDR + 0x10);

	// 1. Truyền Start bit
	*I2C1_CR1 |= (1 << 8);
		// Wait until Start condition generated
		while(((*I2C1_SR1 >> 0 ) & 1) == 0);

	// 2. Gửi địa chỉ của Accelerometer | Write
	*I2C1_DR = (slave_addr << 1) | 0 ;
		// Wait until ADDR bit -> set
		while(((*I2C1_SR1 >> 1) & 1) == 0);
		// Clear bit ADDR
		(void)I2C1->SR2;

	/* 3. Check ACK */
	while (((*I2C1_SR1 >> 10) & 1) == 1); // có lỗi

	/* 4. Gửi địa chỉ của thanh ghi CTRL_REG1_A (0x20)  */
	*I2C1_DR = reg;
		// wait until data is send
		while (((*I2C1_SR1 >> 7) & 1) == 0);

	/* 5. Check ACK */
	while (((*I2C1_SR1 >> 10) & 1) == 1); // có lỗi

	/* 6. Send Start bit again*/
	*I2C1_CR1 |= (1 << 8);
		// Wait until Start condition generated
		while(((*I2C1_SR1 >> 0 ) & 1) == 0);

	// 7. Gửi địa chỉ của Accelerometer | Read bit
	*I2C1_DR = (slave_addr  << 1) | 1 ;
		// Wait until ADDR bit -> set
		while (((*I2C1_SR1 >> 1) & 1) == 0);
		// Clear bit ADDR
		(void)I2C1->SR2;

	/* 8. Clear ACK */
	*I2C1_CR1 &= ~(1 << 10);  // Clear ACK để chuẩn bị nhận 1 byte cuối

	/* 9. Send stop bit CR1:STOP[9] = 1 */
		// Wait until data register has data
		while (((*I2C1_SR1 >> 6) & 1) == 0);
	*I2C1_CR1 |= (1 << 9) ;

	/* 10. Read Data */
	data = *I2C1_DR ;

	return data;

}
int main(){
	HAL_Init();
	I2C_INIT();

	I2C_write_CTRL_REG1_A(0x19, 0x20, 0x57);
	uint8_t val = I2C_read_CTRL_REG1_A(0x19, 0x20);

	while(1){

	}

}
