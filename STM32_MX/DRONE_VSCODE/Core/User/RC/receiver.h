#ifndef RECEIVER_H 
#define RECEIVER_H

#include <stdint.h>
#include <stdbool.h>

void rx_init(void);
uint16_t rx_get_us(uint8_t ch);     // ch = 1 ... RX_PPM_CH_NUM (8)
void rx_get_all_us(uint16_t out[]);
bool rx_is_failsafe(void);  


#endif