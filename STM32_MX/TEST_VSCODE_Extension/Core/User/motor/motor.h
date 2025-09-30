#ifndef MOTOR_H
#define MOTOR_H

#include "board_config.h"

void motor_pwm_init(void);
void motor_pwm_set(uint8_t motor_id, uint16_t pulse_us);

#endif 