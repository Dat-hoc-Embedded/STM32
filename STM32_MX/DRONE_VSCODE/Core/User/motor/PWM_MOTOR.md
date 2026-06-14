# PWM Motor Control Module - Technical Specification

## 📋 Tổng Quan

Module này cung cấp **PWM (Pulse Width Modulation) output** để điều khiển 4 động cơ ESC (Electronic Speed Controller) trên Drone. Dùng **STM32F411 Timer** ở chế độ **Output Compare** để sinh tín hiệu PWM tần số 50Hz với độ rộng xung 1000-2000µs (tuân theo chuẩn ESC/RC).

### Thông Số Chính
| Parameter | Giá trị | Ghi chú |
|-----------|--------|--------|
| **PWM Frequency** | 50 Hz | Period = 20ms (chuẩn RC servo/ESC) |
| **Timer Resolution** | 1 µs | 1 tick = 1 microsecond |
| **Pulse Width Range** | 1000-2000 µs | Tương ứng: Tắt - Full Speed |
| **Số kênh** | 4 channels | Motor 1,2,3,4 |
| **Timer sử dụng** | TIM4 | STM32F411VETx |
| **GPIO** | PB6, PB7, PB8, PB9 | Alternate Function AF2 |

---

## 🏗️ Architecture

### Block Diagram
```
┌─────────────────────────────────────────────────┐
│         STM32F411 TIMER4 (100 MHz)              │
├─────────────────────────────────────────────────┤
│                                                 │
│  ┌─────────────┐      ┌──────────────────┐    │
│  │ Prescaler   │──→   │  ARR (20000-1)   │    │
│  │ PSC = 99    │      │  Period = 20ms   │    │
│  └─────────────┘      └──────────────────┘    │
│         ↓                      ↓               │
│    100MHz÷100 = 1MHz    Overflow @ 50Hz      │
│                                                 │
│  ┌──────────────────────────────────────┐     │
│  │    Channel 1  Channel 2  Channel 3  │     │
│  │    (PB6)      (PB7)     (PB8)       │     │
│  │                                      │     │
│  │  Output Compare Mode 1 (PWM Mode 1) │     │
│  │  Compare Register: CCR1, CCR2, CCR3 │     │
│  │  Value: 1000-2000                   │     │
│  └──────────────────────────────────────┘     │
│         ↓         ↓         ↓                  │
│      GPIO Output (50Hz PWM Signal)            │
│                                                 │
└─────────────────────────────────────────────────┘

┌────────────────────────────────────────────────┐
│         Channel 4 (PB9)                        │
│    Output Compare Mode 1 (PWM Mode 1)         │
│    Compare Register: CCR4                      │
└────────────────────────────────────────────────┘
```

### Module Structure
```
motor_pwm_init()
    │
    ├─→ gpio_pwm_init()          # GPIO Configuration
    │       ├─→ Enable Clock (GPIOB)
    │       ├─→ Set GPIO Mode = Alternate Function
    │       └─→ Set Alternate Function = AF2 (Timer4)
    │
    └─→ timer_pwm_init()         # Timer Configuration
            ├─→ Enable Timer Clock
            ├─→ Prescaler Setup (PSC = 99)
            ├─→ Period Setup (ARR = 19999)
            ├─→ Output Compare Mode Config (CCMR1, CCMR2)
            ├─→ Output Enable (CCER)
            └─→ Start Timer (CEN)

motor_pwm_set(motor_id, pulse_us)
    └─→ Set Compare Register (CCR1/CCR2/CCR3/CCR4)
        └─→ Pulse Width = 1000-2000 µs
```

---

## 🔧 Timer Output Compare Configuration Steps

### Bước 1: Clock Enable
```c
MOTOR_TIMER_CLK_EN();      // Enable Timer4 clock
MOTOR_GPIO_CLK_ENB();      // Enable GPIOB clock
```
**Mục đích**: Cho phép Timer4 và GPIOB hoạt động

---

### Bước 2: GPIO Configuration (Alternate Function Mode)
```c
/* 2a. Set GPIO Mode = Alternate Function (AF) cho PB6, PB7, PB8, PB9 */
MOTOR_GPIO_PORT->MODER &= ~((3 << (MOTOR1_PIN * 2)) | 
                             (3 << (MOTOR2_PIN * 2)) | 
                             (3 << (MOTOR3_PIN * 2)) |
                             (3 << (MOTOR4_PIN * 2)));

MOTOR_GPIO_PORT->MODER |= ((2 << (MOTOR1_PIN * 2)) | 
                            (2 << (MOTOR2_PIN * 2)) |
                            (2 << (MOTOR3_PIN * 2)) | 
                            (2 << (MOTOR4_PIN * 2)));
/* Mode = 2 → Alternate Function
   Bit field: MODER[2n:2n+1]
   00: Input
   01: General Purpose Output (GPIO)
   10: Alternate Function (Timer)
   11: Analog
*/

/* 2b. Set Alternate Function = AF2 (Timer4) */
MOTOR_GPIO_PORT->AFR[0] |= ((MOTOR1_AF << (MOTOR1_PIN * 4)) | 
                             (MOTOR2_AF << (MOTOR2_PIN * 4)));
/* AFR[0] cho PIN 0-7 (PB6, PB7) */

MOTOR_GPIO_PORT->AFR[1] |= ((MOTOR3_AF << ((MOTOR3_PIN - 8) * 4)) | 
                             (MOTOR4_AF << ((MOTOR4_PIN - 8) * 4)));
/* AFR[1] cho PIN 8-15 (PB8, PB9) */
```

**Mục đích**: 
- MODER = 2 để chọn Alternate Function
- AFR = 2 để chọn Timer4 (AF2)
- Pin PB6/PB7 → Timer4 CH1/CH2
- Pin PB8/PB9 → Timer4 CH3/CH4

---

### Bước 3: Timer Prescaler & Period Setup
```c
uint32_t F_sys = HAL_RCC_GetSysClockFreq();  // 100 MHz

/* 3a. Prescaler: Tạo 1 MHz tick (1 µs resolution) */
MOTOR_TIMER->PSC = (F_sys / 1e6) - 1;        // = 99
/*
   Timer Clock Input = 100 MHz
   Prescaler = PSC + 1 = 100
   Timer Frequency = 100 MHz / 100 = 1 MHz
   → 1 tick = 1 µs ✓
*/

/* 3b. Auto-Reload Register: Period = 20ms (50 Hz) */
MOTOR_TIMER->ARR = (1e6 / PWM_FREQUENCY_HZ) - 1;  // = 19999
/*
   Period = (ARR + 1) × (1/Timer_Freq)
   Period = 20000 × (1/1MHz) = 20000 µs = 20 ms
   Frequency = 1/20ms = 50 Hz ✓
*/
```

**Mục đích**:
- Prescaler giảm 100MHz → 1MHz (1µs resolution)
- ARR = 19999 sinh tần số 50Hz (20ms period)
- Timer overflow mỗi 20ms

---

### Bước 4: Output Compare Mode Configuration (PWM)
```c
/* 4a. CCMR1: Configure Channel 1 & Channel 2 */
MOTOR_TIMER->CCMR1 |= ((6 << TIM_CCMR1_OC1M_Pos) | 
                        (1 << TIM_CCMR1_OC1PE_Pos) |
                        (6 << TIM_CCMR1_OC2M_Pos) | 
                        (1 << TIM_CCMR1_OC2PE_Pos));

/* 
   OC1M[2:0] = 110 → Mode 6 = PWM Mode 1
   OC1PE[0] = 1     → Enable Preload (ARR update)
*/

/* 4b. CCMR2: Configure Channel 3 & Channel 4 */
MOTOR_TIMER->CCMR2 |= ((6 << TIM_CCMR2_OC3M_Pos) | 
                        (1 << TIM_CCMR2_OC3PE_Pos) |
                        (6 << TIM_CCMR2_OC4M_Pos) | 
                        (1 << TIM_CCMR2_OC4PE_Pos));
```

**PWM Mode 1 (Mode 6) Behavior**:
- Output = HIGH khi: Counter < CCR (Compare Value)
- Output = LOW khi: Counter ≥ CCR
- Duty Cycle = CCR / ARR × 100%

**Ví dụ với CCR = 1500µs**:
```
Timeline (1 period = 20ms):
|←─ 1500µs ─→|←─ 18500µs ─→|
|  HIGH (ON) |  LOW (OFF)  |
├─────────────┼─────────────┤
0            1500         20000

Pulse Width = 1500µs ✓
Duty = 1500/20000 = 7.5%
```

---

### Bước 5: Output Enable (CCER)
```c
/* Enable Output Channels */
MOTOR_TIMER->CCER |= ((1 << TIM_CCER_CC1E_Pos) | 
                       (1 << TIM_CCER_CC2E_Pos) |
                       (1 << TIM_CCER_CC3E_Pos) | 
                       (1 << TIM_CCER_CC4E_Pos));
/*
   CC1E = 1 → Enable Channel 1 Output
   CC2E = 1 → Enable Channel 2 Output
   CC3E = 1 → Enable Channel 3 Output
   CC4E = 1 → Enable Channel 4 Output
*/
```

**Mục đích**: Bật output pin cho các channel

---

### Bước 6: Auto-Reload Preload & Timer Start
```c
/* Auto-Reload Preload: Update ARR từ Shadow Register */
MOTOR_TIMER->CR1 |= TIM_CR1_ARPE;
/*
   ARPE = 1 → ARR được load từ preload register
   Điều này đảm bảo ARR được update đúng timing
*/

/* Generate Update Event (BG) */
MOTOR_TIMER->EGR |= TIM_EGR_BG;
/*
   BG = 1 → Generate Update Event
   Reset PSC, ARR, và counter
*/

/* Initialize PWM Values */
MOTOR1_SPEED = PWM_MIN_US;  // 1000 µs
MOTOR2_SPEED = PWM_MIN_US;
MOTOR3_SPEED = PWM_MIN_US;
MOTOR4_SPEED = PWM_MIN_US;

/* Start Timer */
MOTOR_TIMER->CR1 |= TIM_CR1_CEN;
/*
   CEN = 1 → Start counting
*/
```

**Mục đích**: 
- Khởi tạo timer state
- Bắt đầu đếm xung đồng hồ

---

## ⚙️ PWM Control API

### Function: `motor_pwm_set()`
```c
void motor_pwm_set(uint8_t motor_id, uint16_t pulse_us)
{
    /* Boundary Check */
    if (pulse_us > PWM_MAX_US) pulse_us = PWM_MAX_US;  // 2000µs
    if (pulse_us < PWM_MIN_US) pulse_us = PWM_MIN_US;  // 1000µs

    /* Set Compare Register */
    switch (motor_id) {
        case 1: MOTOR1_SPEED = pulse_us;  // CCR1
                break;
        case 2: MOTOR2_SPEED = pulse_us;  // CCR2
                break;
        case 3: MOTOR3_SPEED = pulse_us;  // CCR3
                break;
        case 4: MOTOR4_SPEED = pulse_us;  // CCR4
                break;
        default: break;
    }
}
```

**Parameters**:
- `motor_id`: 1-4 (Motor số)
- `pulse_us`: 1000-2000 (Pulse width µs)

**Ảnh hưởng**:
- CCRx = pulse_us → Thay đổi duty cycle
- PWM output ngay lập tức (vì có ARPE preload)

---

## 📊 Timing Diagram

### PWM Output Waveform (50Hz, 20ms Period)
```
Pulse Width = 1000µs (Min Speed):
│
│ ┌─────┐
├─┤     └─────────────────────────┬──────
│ 0    1000                      20000   20ms
│ ←─────┘  ←───── 19000µs ──────→
│ ↑ HIGH     ↑ LOW

Pulse Width = 1500µs (50% Speed):
│
│ ┌──────────┐
├─┤          └──────────────┬──────
│ 0        1500           20000   40ms
│ ←──────┘  ←── 18500µs ──→
│ ↑ HIGH     ↑ LOW

Pulse Width = 2000µs (Max Speed):
│
│ ┌──────────────────┐
├─┤                  └──┬──────
│ 0              2000  20000   60ms
│ ←──────────────┘  ↑
│ ↑ HIGH       ↑ LOW
│ 20ms Period   ← (100% duty)
```

### ESC Signal Interpretation
```
Pulse Width → Motor Speed
┌──────────────────┬──────────────┐
│   Pulse (µs)     │ Motor State  │
├──────────────────┼──────────────┤
│   1000 µs        │ Stopped      │
│   1500 µs        │ 50% Speed    │
│   2000 µs        │ Full Speed   │
└──────────────────┴──────────────┘
```

---

## 📋 Register Reference

### Key Registers Used

| Register | Bits | Purpose |
|----------|------|---------|
| **PSC** | [15:0] | Prescaler = 99 (divide by 100) |
| **ARR** | [15:0] | Auto-Reload = 19999 (20ms period) |
| **CCMR1** | [2:0], [10:8] | Mode 6 (PWM) for CH1, CH2 |
| **CCMR2** | [2:0], [10:8] | Mode 6 (PWM) for CH3, CH4 |
| **CCR1-4** | [15:0] | Compare Values (1000-2000) |
| **CCER** | [0,4,8,12] | Enable Outputs (CC1E-CC4E) |
| **CR1** | [0], [7] | CEN (Start), ARPE (Preload) |
| **EGR** | [0] | Generate Update Event (UG) |

---

## 🔍 Troubleshooting

### Problem: PWM không phát ra tín hiệu
**Nguyên nhân tiềm tàng**:
1. ❌ Timer clock chưa bật → `MOTOR_TIMER_CLK_EN()` chưa gọi
2. ❌ GPIO mode chưa cấu hình → Vẫn ở mode GPIO, không phải AF
3. ❌ CCER chưa bật → Output chưa enable
4. ❌ CR1 CEN = 0 → Timer chưa start

**Kiểm tra**:
```c
// Debug: Check registers
printf("CR1: 0x%X (CEN bit should = 1)\n", MOTOR_TIMER->CR1 & 0x1);
printf("CCER: 0x%X (CC1E-CC4E should = 1)\n", MOTOR_TIMER->CCER & 0x1111);
printf("PSC: %d (Should be 99)\n", MOTOR_TIMER->PSC);
printf("ARR: %d (Should be 19999)\n", MOTOR_TIMER->ARR);
printf("MODER: 0x%X (Should be 0x2 for AF)\n", MOTOR_GPIO_PORT->MODER);
```

### Problem: PWM frequency sai (không phải 50Hz)
**Nguyên nhân**:
1. ❌ PSC sai → Tính lại từ system clock
2. ❌ ARR sai → Phải = 19999 cho 50Hz

**Công thức**:
```
PSC = (System_Clock / Desired_Freq) - 1
ARR = (Desired_Freq / PWM_Frequency) - 1

Ví dụ (100MHz, 1MHz timer, 50Hz PWM):
PSC = (100MHz / 1MHz) - 1 = 99 ✓
ARR = (1MHz / 50Hz) - 1 = 19999 ✓
```

### Problem: Pulse width không thay đổi
**Nguyên nhân**:
1. ❌ CCRx không được update → Check `motor_pwm_set()` gọi đúng
2. ❌ Giá trị pulse_us ngoài range → Boundary check

**Kiểm tra**:
```c
printf("CCR1: %d (Should be 1000-2000)\n", MOTOR_TIMER->CCR1);
printf("CCR2: %d\n", MOTOR_TIMER->CCR2);
printf("CCR3: %d\n", MOTOR_TIMER->CCR3);
printf("CCR4: %d\n", MOTOR_TIMER->CCR4);
```

---

## 📝 Usage Example

```c
int main(void) {
    /* Initialize System */
    SystemInit();
    
    /* Initialize Motor PWM */
    motor_pwm_init();  // Cài đặt timer, GPIO, PWM
    
    /* Control Motors */
    motor_pwm_set(1, 1000);   // Motor 1: Min speed (stopped)
    motor_pwm_set(2, 1500);   // Motor 2: 50% speed
    motor_pwm_set(3, 2000);   // Motor 3: Max speed
    motor_pwm_set(4, 1200);   // Motor 4: Low speed
    
    /* Ramp up motor 1 */
    for (uint16_t pulse = 1000; pulse <= 2000; pulse += 10) {
        motor_pwm_set(1, pulse);
        HAL_Delay(50);  // 50ms delay
    }
    
    while (1) {
        /* Main loop */
    }
}
```

---

## 📚 References

- **STM32F411 Reference Manual**: Section 16 (General-Purpose Timers)
  - Timer Configuration (PSC, ARR, CCR)
  - Output Compare Mode
  - PWM Output

- **RC/ESC Standards**:
  - PPM: 1000µs = Min, 1500µs = Mid, 2000µs = Max
  - Frequency: 50Hz (20ms period) standard

- **ARM CMSIS Registers**:
  - `TIM_CCMR1_OC1M_Pos`: Output Compare 1 Mode
  - `TIM_CCER_CC1E_Pos`: Capture/Compare 1 Output Enable

---

**Document Version**: 1.0  
**Date**: 2026-06-15  
**Author**: embeddat  
**Status**: Final
