# STM32F411 Clock Configuration - Technical Guide

## 📋 Tổng Quan

File `main.c` chứa các hàm cấu hình clock **STM32F411VETx** từ mặc định **16 MHz (HSI)** lên **100 MHz (PLL từ HSE 8 MHz)**.

---

## 🕐 Clock Architecture

### Mặc Định (HSI Mode - Nếu KHÔNG gọi `Clock_Init()`)

```
External Clock Sources:
├─ HSI (High-Speed Internal): 16 MHz ✓ (always available)
├─ HSE (High-Speed External):  8 MHz  (requires crystal)
└─ LSI (Low-Speed Internal):   32 kHz

Default Boot Path (Without Clock_Init):
┌─────────────────┐
│   HSI 16 MHz    │
└────────┬────────┘
         │
    No PLL enabled
         │
    ┌────▼─────────┐
    │ System Clock │
    │   16 MHz     │ ← Directly from HSI
    └────┬─────────┘
         │
    ┌────▼──────┬────────────┬──────────┐
    │ HCLK       │ PCLK1      │ PCLK2    │
    │ 16 MHz ÷1  │ 16 MHz ÷1  │ 16 MHz ÷1│
    └────────────┴────────────┴──────────┘
```

| Parameter | Default (HSI) | Configured (PLL) | Tỷ lệ |
|-----------|---------------|------------------|-------|
| System Clock | 16 MHz | 100 MHz | **6.25x** |
| HCLK | 16 MHz | 100 MHz | 6.25x |
| PCLK1 | 16 MHz | 50 MHz | 3.125x |
| PCLK2 | 16 MHz | 100 MHz | 6.25x |
| Timer Resolution | 62.5 ns | 10 ns | **6.25x chính xác hơn** |

---

### Hiện Tại (Với `Clock_Init()`)

```
PLL Configuration Path (With Clock_Init):

┌──────────────────┐
│ HSE 8 MHz        │ (External Crystal)
└────────┬─────────┘
         │
    ┌────▼─────────────┐
    │  PLLM (÷8)       │
    │  8 MHz ÷ 8 = 1 MHz
    └────┬─────────────┘
         │
    ┌────▼──────────────┐
    │  PLLN (×200)      │
    │  1 MHz × 200 = 200 MHz (VCO)
    └────┬──────────────┘
         │
    ┌────▼──────────────┐
    │  PLLP (÷2)        │
    │  200 MHz ÷ 2 = 100 MHz ← MAIN OUTPUT
    └────┬──────────────┘
         │
    ┌────▼────────────┐
    │ System Clock    │
    │   100 MHz ✓     │
    └────┬────────────┘
         │
    ┌────▼──────┬────────────┬──────────┐
    │ HCLK       │ PCLK1      │ PCLK2    │
    │ 100 MHz÷1  │ 100 MHz ÷2 │ 100 MHz÷1│
    │ (100 MHz)  │ (50 MHz)   │(100 MHz) │
    └────────────┴────────────┴──────────┘
```

---

## 🔧 6 Bước Cài Đặt Clock Chi Tiết

### Bước 1: Enable HSE (High-Speed External)
```c
RCC->PLLCFGR |= RCC_PLLCFGR_PLLSRC_HSE;

RCC->CR |= RCC_CR_HSEON;
while (!(RCC->CR & RCC_CR_HSERDY));  // Wait until HSE is ready
```
**Mục đích**: Chọn HSE (8 MHz external crystal) làm nguồn cho PLL

---

### Bước 2: Cấu hình PLL Dividers & Multipliers
```c
RCC->PLLCFGR &= ~(RCC_PLLCFGR_PLLM_Msk | 
                  RCC_PLLCFGR_PLLN_Msk | 
                  RCC_PLLCFGR_PLLP_Msk);

RCC->PLLCFGR |= ((8 << RCC_PLLCFGR_PLLM_Pos) |   // PLLM = 8 (÷8)
                 (200 << RCC_PLLCFGR_PLLN_Pos) | // PLLN = 200 (×200)
                 (0 << RCC_PLLCFGR_PLLP_Pos));   // PLLP = 0 (÷2)
```

**Công thức**:
```
f_VCO = f_HSE × (PLLN / PLLM)
      = 8 MHz × (200 / 8)
      = 8 MHz × 25
      = 200 MHz

f_PLL_Output = f_VCO / (PLLP + 1)
             = 200 MHz / 2
             = 100 MHz ✓
```

| Parameter | Giá trị | Ý nghĩa |
|-----------|--------|---------|
| PLLM | 8 | Chia 8: 8 MHz → 1 MHz |
| PLLN | 200 | Nhân 200: 1 MHz → 200 MHz (VCO) |
| PLLP | 0 | Chia (2^(0+1)) = 2: 200 MHz → 100 MHz |

---

### Bước 3: Set Flash Latency (Wait States)
```c
FLASH->ACR &= ~FLASH_ACR_LATENCY_Msk;
FLASH->ACR |= FLASH_ACR_LATENCY_3WS;
```

**Tại sao cần thiết**?
- Flash memory chậm hơn CPU
- CPU @ 100 MHz cần chờ 3 cycle để đọc Flash
- Nếu không set → lỗi dữ liệu, code không chạy đúng

| System Clock | Min Wait States | Vdd |
|--------------|-----------------|-----|
| 0-30 MHz | 0 WS | >= 2.7V |
| 30-60 MHz | 1 WS | >= 2.7V |
| 60-90 MHz | 2 WS | >= 2.7V |
| 90-120 MHz | 3 WS | >= 2.7V |

**Ở đây**: 100 MHz → 3 WS ✓

---

### Bước 4: Enable PLL Clock
```c
RCC->CR |= RCC_CR_PLLON;
while (!(RCC->CR & RCC_CR_PLLRDY));  // Wait until PLL is ready
```

**Mục đích**: Bật PLL và chờ nó ổn định

---

### Bước 5: Cấu hình Prescalers
```c
/* AHB Prescaler = /1 */
RCC->CFGR &= ~RCC_CFGR_HPRE;  // Keep default /1

/* APB1 Prescaler = /2 (Max 50 MHz) */
RCC->CFGR &= ~RCC_CFGR_PPRE1_Msk;
RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;

/* APB2 Prescaler = /1 (Max 100 MHz) */
RCC->CFGR &= ~RCC_CFGR_PPRE2_Msk;
RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;
```

| Bus | Prescaler | Output | Max | Devices |
|-----|-----------|--------|-----|---------|
| AHB | ÷1 | 100 MHz | 100 MHz | Cortex-M4, DMA |
| APB1 | ÷2 | 50 MHz | 50 MHz | UART1, I2C, SPI |
| APB2 | ÷1 | 100 MHz | 100 MHz | ADC, GPIO, Timer4 |

**⚠️ Quan trọng**: APB1 có giới hạn max 50 MHz, nên ÷2 là bắt buộc!

---

### Bước 6: Switch System Clock to PLL
```c
RCC->CFGR &= ~RCC_CFGR_SW;  // Clear SW bits
RCC->CFGR |= RCC_CFGR_SW_PLL;  // Select PLL as SYSCLK
while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);  // Wait for switch
```

**Mục đích**: Thay đổi source clock từ HSI (16 MHz) sang PLL (100 MHz)

---

## 📊 Debug Output

### Chạy `Clock_Debug_Print()`
```
========== CLOCK CONFIGURATION DEBUG ==========
HCLK (AHB):         100000000 Hz (100 MHz)
PCLK1 (APB1):       50000000 Hz (50 MHz)  [÷2 Prescaler]
PCLK2 (APB2):       100000000 Hz (100 MHz)  [÷1 Prescaler]
SystemCoreClock:    100000000 Hz (100 MHz)
SysTick Clock:      100000000 Hz (100 MHz)
============================================
```

### Chạy `Clock_Setup_Explanation()`
In chi tiết 6 bước setup + register values hiện tại

### Chạy `Clock_Compare_Default()`
So sánh DEFAULT (16 MHz) vs CONFIGURED (100 MHz):
- Độ phân giải Timer: 62.5ns → 10ns (6.25x chính xác)
- UART baud: Không chính xác → Chính xác
- Motor PWM: Sai tần số → 50Hz chính xác ✓
- Performance: 6.25x chậm hơn → 6.25x nhanh hơn

---

## 🔍 Vai Trò Từng Hàm trong main()

```c
int main(){
    // 1️⃣  HAL_Init() 
    //    - Khởi tạo HAL library
    //    - Setup SysTick (1ms timer)
    //    - Cấu hình interrupt priorities
    
    // 2️⃣  SystemClock_Config()
    //    - Cấu hình PLL lên 100 MHz (6 bước như trên)
    //    - Nếu KHÔNG: CPU chạy 16 MHz (quá chậm!)
    
    // 3️⃣  SystemCoreClockUpdate()
    //    - Cập nhật biến global SystemCoreClock
    //    - HAL_Delay() dùng nó để đếm chính xác
    //    - Nếu KHÔNG: HAL_Delay(1000) sẽ lâu hơn 1 giây!
    
    // 4️⃣  HAL_InitTick(TICK_INT_PRIORITY)
    //    - Khởi tạo SysTick theo SystemCoreClock mới
    //    - Đồng bộ SysTick với clock 100MHz
    //    - Nếu KHÔNG: SysTick vẫn ở 16MHz setting
    
    // 5️⃣  UART1_INIT()
    //    - Khởi tạo UART với PCLK1 = 50 MHz
    //    - Baud rate 115200 sẽ chính xác
    
    // 6️⃣  motor_pwm_init()
    //    - Khởi tạo PWM với PCLK2 = 100 MHz
    //    - Timer4 frequency: 50 Hz (20ms) ✓
    //    - Nếu KHÔNG có Clock_Init: PWM frequency sẽ SAIIII!
    
    // 7️⃣  rx_init()
    //    - Khởi tạo receiver (dùng timers, interrupts)
    //    - Timing chính xác vì đã cấu hình clock
}
```

---

## ⚠️ Nếu KHÔNG gọi `Clock_Init()`

### Kết quả: STM32 chạy ở DEFAULT MODE
```
Điều gì sẽ xảy ra:
┌────────────────────────────────────────────────────────┐
│ ❌ System Clock: 16 MHz (từ HSI, không qua PLL)       │
│ ❌ PCLK1, PCLK2: 16 MHz                                │
│ ❌ Timer Resolution: 62.5 ns (thay vì 10 ns)          │
│ ❌ Motor PWM Frequency: ~312.5 Hz (thay vì 50 Hz)     │
│ ❌ UART Baud Rate: Sai tỷ lệ                          │
│ ❌ HAL_Delay: Chạy chậm 6.25x                         │
│ ❌ Performance: Yếu, không đủ để xử lý interrupt      │
└────────────────────────────────────────────────────────┘

Motor PWM Tính Toán (Nếu không cấu hình):
PSC = (16 MHz / 1 MHz) - 1 = 15
ARR = 20000 - 1
Frequency = 16 MHz / 16 / 20000 = 50 Hz ??? 

LỖII!!! Vì:
- Prescaler KHÔNG được điều chỉnh
- Frequency = 16 MHz / 20000 = 800 Hz (quá cao!)
- Motor sẽ quay hàng trăm lần một giây (nguy hiểm!)
```

---

## 📋 Clock Variables Available for Debug

```c
/* Global struct chứa tất cả clock values */
typedef struct {
    uint32_t HCLK;              // AHB Clock
    uint32_t PCLK1;             // APB1 Clock  
    uint32_t PCLK2;             // APB2 Clock
    uint32_t SystemCoreClock;   // System Clock Variable
    uint32_t SysTickClock;      // SysTick Frequency
} ClockConfig_t;

extern ClockConfig_t current_clocks;  // Access từ bất kỳ file nào

/* Sử dụng */
printf("HCLK = %lu MHz\n", current_clocks.HCLK / 1000000);
printf("PCLK1 = %lu MHz\n", current_clocks.PCLK1 / 1000000);
```

---

## 🔧 Các Register Cần Biết

| Register | Mục đích |
|----------|---------|
| `RCC->PLLCFGR` | PLL Configuration (PLLM, PLLN, PLLP) |
| `RCC->CR` | Clock Control (HSEON, PLLON, HSERDY, PLLRDY) |
| `RCC->CFGR` | Clock Configuration (Prescalers, Clock Source) |
| `FLASH->ACR` | Flash Access Control (Latency/Wait States) |
| `SystemCoreClock` | Global variable chứa system clock frequency |

---

## 📚 References

- STM32F411 Reference Manual - Section 3.3 (Clocking)
- STM32CubeMX HAL Documentation
- ARM CMSIS Headers

---

**Version**: 1.0  
**Date**: 2026-06-15  
**Author**: STM32 Development Team
