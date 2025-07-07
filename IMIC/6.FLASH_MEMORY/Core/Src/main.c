#include <main.h>

#define FLASH_INTERFACE_BASE_ADDR 0x40023C00

void flash_erase_sector (int sec_num){
	uint32_t *FLASH_SR = (uint32_t*)(FLASH_INTERFACE_BASE_ADDR + 0x0C);
	uint32_t *FLASH_CR = (uint32_t*)(FLASH_INTERFACE_BASE_ADDR + 0x10);

	if((*FLASH_CR >> 31) == 1){
		uint32_t* FLASH_KEYR = (FLASH_INTERFACE_BASE_ADDR + 0x04);
		*FLASH_KEYR = 0x45670123;
		*FLASH_KEYR = 0xCDEF89AB;
	}
	if(sec_num > 7){
		return;
	}
	// Check BSY bit in SR register, nếu FLASH bận thì BSY = 1 mình sẽ chờ
	while (((*FLASH_SR >> 16) & 1) == 1);

	// Set SER bit & SNB bit in CR register
	*FLASH_CR |= 1 << 1;
	*FLASH_CR |= (sec_num << 3);

	// Set the STRT bit in the FLASH_CR register
	*FLASH_CR |= (1 << 16);

	// Wait for the BSY bit to be clear
	while(((*FLASH_SR >> 16) & 1) == 1);

}
void flash_program(uint16_t* addr, uint16_t val){
	uint32_t *FLASH_SR = (uint32_t*)(FLASH_INTERFACE_BASE_ADDR + 0x0C);
	uint32_t *FLASH_CR = (uint32_t*)(FLASH_INTERFACE_BASE_ADDR + 0x10);

	if((*FLASH_CR >> 31) == 1){
		// unlock by the unlock sequence
		uint32_t* FLASH_KEYR = FLASH_INTERFACE_BASE_ADDR + 0x04;
		*FLASH_KEYR = 0x45670123;
		*FLASH_KEYR = 0xCDEF89AB;
	}
	// Check BSY bit in the FLASH_SR register
	while (((*FLASH_SR >> 16) & 1) == 1);

	// SET the PG bit in the FLASH_CR register.
	*FLASH_CR |= 1 << 0;

	// Ghi dữ liệu vào địa chỉ memory.
	*addr = val;

	// Wait for the BSY bit to be cleared.
	while(((*FLASH_SR >> 16) & 1) == 1);
}
int main(){
	flash_erase_sector(1);
	flash_program(0x08004000 ,'xe');
	while(1){

	}
}
