# HSI vs PLL - Cách Kiểm tra Default Clock State

## 🎯 Câu Hỏi: Làm sao biết nếu K cấu hình thì sẽ là HSI 16MHz?

### Cách 1: Dùng Hàm `Clock_GetSource()` trong Code

```c
const char* Clock_GetSource(void)
{
    uint32_t sws = (RCC->CFGR & RCC_CFGR_SWS) >> RCC_CFGR_SWS_Pos;
    
    switch (sws) {
        case 0: return "HSI (16 MHz) ⚠️  DEFAULT";  // 00 = HSI
        case 1: return "HSE (8 MHz)";                // 01 = HSE
        case 2: return "PLL (100 MHz) ✅";           // 10 = PLL
        case 3: return "PLLI2S";                     // 11 = PLLI2S
        default: return "Unknown";
    }
}
```

**Cách sử dụng:**
```c
printf("Current Clock Source: %s\n", Clock_GetSource());

// Output nếu K cấu hình:
// "HSI (16 MHz) ⚠️  DEFAULT"

// Output nếu đã cấu hình:
// "PLL (100 MHz) ✅"
```

---

### Cách 2: Kiểm tra `RCC->CFGR` Register

**Register Bits:**
```c
/* Kiểm tra bit SWS (System Clock Source) trong RCC->CFGR */
uint32_t sws = (RCC->CFGR & 0xC) >> 2;  // Lấy bit [3:2]

if (sws == 0) {
    printf("Using HSI 16 MHz ⚠️\n");    // Default
}
else if (sws == 2) {
    printf("Using PLL 100 MHz ✅\n");   // Configured
}
```

**Ý nghĩa:**
```
SWS Bits [3:2] | Clock Source
─────────────────────────────
00             | HSI (16 MHz) ← DEFAULT nếu K cấu hình
01             | HSE (8 MHz)
10             | PLL (100 MHz) ← CONFIGURED
11             | PLLI2S
```

---

### Cách 3: Dùng Hàm `Clock_PrintRawRegisters()`

In ra **toàn bộ RCC register values** để thấy cấu hình hiện tại:

```c
Clock_PrintRawRegisters();
```

**Output:**
```
RCC->CR (Control Register):        0x........
  └─ HSION (HSI Enable):           1
  └─ HSERDY (HSE Ready):           1
  └─ HSEON (HSE Enable):           1
  └─ PLLON (PLL Enable):           1
  └─ PLLRDY (PLL Ready):           1

RCC->CFGR (Config Register):       0x........
  └─ SWS (Clock Source):           0x2 (PLL (100 MHz) ✅)
  └─ HPRE (AHB Prescaler):         0
  └─ PPRE1 (APB1 Prescaler):       4
  └─ PPRE2 (APB2 Prescaler):       0
```

---

### Cách 4: Dùng Hàm `Clock_ShowDefaultState()`

Hàm này **in ra thông tin chi tiết** về nếu K cấu hình:

```c
Clock_ShowDefaultState();
```

**Output:**
```
╔════════════════════════════════════════════════════════════╗
║          DEFAULT CLOCK STATE (NẾU K CẤU HÌNH)             ║
╚════════════════════════════════════════════════════════════╝

Nếu CHỈ gọi HAL_Init() mà KHÔNG gọi SystemClock_Config():

├─ Clock Source: HSI (High-Speed Internal Oscillator)
├─ HSI Frequency: 16 MHz (±1% accuracy)
├─ PLL Status: DISABLED ❌
├─ System Clock: 16 MHz (direct from HSI)
└─ Prescalers: /1 (no prescaling)

📊 Default Clock Frequencies (WITHOUT Clock_Init):
┌─────────────────────────────────────────────────────────┐
│ HCLK (AHB):      16 MHz                                  │
│ PCLK1 (APB1):    16 MHz                                  │
│ PCLK2 (APB2):    16 MHz                                  │
│ SystemCoreClock: 16 MHz                                  │
└─────────────────────────────────────────────────────────┘

⚠️  PROBLEMS Without Clock_Init():
...chi tiết các vấn đề sẽ gặp...
```

---

## 📋 Tất Cả Các Hàm Debug Có Sẵn

| Hàm | Mục đích |
|-----|---------|
| `Clock_GetSource()` | Lấy string tên clock source hiện tại |
| `Clock_PrintRawRegisters()` | In raw RCC register values |
| `Clock_UpdateDebugValues()` | Cập nhật struct `current_clocks` |
| `Clock_Debug_Print()` | In tất cả clock frequencies |
| `Clock_ShowDefaultState()` | In thông tin DEFAULT (HSI 16MHz) |
| `Clock_Setup_Explanation()` | In chi tiết 6 bước setup |
| `Clock_Compare_Default()` | So sánh DEFAULT vs CONFIGURED |

---

## 🔧 Code Snippet để Kiểm Tra

### Snippet 1: In Current Clock Source
```c
#include <stdio.h>
#include "stm32f4xx.h"

int main(void) {
    HAL_Init();
    
    // Kiểm tra clock source (vẫn là HSI 16MHz nếu k gọi SystemClock_Config)
    uint32_t sws = (RCC->CFGR & RCC_CFGR_SWS) >> RCC_CFGR_SWS_Pos;
    
    if (sws == 0) {
        printf("⚠️  DEFAULT: Using HSI 16 MHz\n");
        printf("HCLK = 16 MHz, PCLK1 = 16 MHz, PCLK2 = 16 MHz\n");
    }
    
    SystemClock_Config();       // Cấu hình PLL
    SystemCoreClockUpdate();    // Update clock
    
    // Kiểm tra lại
    sws = (RCC->CFGR & RCC_CFGR_SWS) >> RCC_CFGR_SWS_Pos;
    
    if (sws == 2) {
        printf("✅ CONFIGURED: Using PLL 100 MHz\n");
        printf("HCLK = 100 MHz, PCLK1 = 50 MHz, PCLK2 = 100 MHz\n");
    }
}
```

### Snippet 2: Dùng HAL Functions
```c
int main(void) {
    HAL_Init();
    
    // Nếu chỉ HAL_Init(), không cấu hình clock
    uint32_t hclk_before = HAL_RCC_GetHCLKFreq();
    uint32_t pclk1_before = HAL_RCC_GetPCLK1Freq();
    
    printf("BEFORE: HCLK = %lu Hz, PCLK1 = %lu Hz\n", hclk_before, pclk1_before);
    // Output: HCLK = 16000000 Hz, PCLK1 = 16000000 Hz ← DEFAULT HSI
    
    // Sau khi cấu hình
    SystemClock_Config();
    SystemCoreClockUpdate();
    
    uint32_t hclk_after = HAL_RCC_GetHCLKFreq();
    uint32_t pclk1_after = HAL_RCC_GetPCLK1Freq();
    
    printf("AFTER: HCLK = %lu Hz, PCLK1 = %lu Hz\n", hclk_after, pclk1_after);
    // Output: HCLK = 100000000 Hz, PCLK1 = 50000000 Hz ← PLL 100MHz
}
```

### Snippet 3: Kiểm tra Từng Register
```c
int main(void) {
    HAL_Init();
    
    printf("=== RCC Registers ===\n");
    printf("RCC->CR = 0x%lX\n", RCC->CR);      // Enable/Ready bits
    printf("RCC->CFGR = 0x%lX\n", RCC->CFGR);  // Clock source + prescalers
    printf("RCC->PLLCFGR = 0x%lX\n", RCC->PLLCFGR); // PLL config
    
    // Kiểm tra SWS bits
    uint32_t sws = (RCC->CFGR >> 2) & 0x3;
    printf("SWS = 0x%lX ", sws);
    
    if (sws == 0) printf("(HSI 16 MHz - DEFAULT)\n");
    else if (sws == 2) printf("(PLL 100 MHz - CONFIGURED)\n");
}
```

---

## 📊 So Sánh Output

### BEFORE (Chỉ gọi HAL_Init())
```
Current Clock Source: HSI (16 MHz) ⚠️  DEFAULT
HCLK (AHB):         16000000 Hz (16 MHz)
PCLK1 (APB1):       16000000 Hz (16 MHz)  [÷1 Prescaler]
PCLK2 (APB2):       16000000 Hz (16 MHz)  [÷1 Prescaler]
SystemCoreClock:    16000000 Hz (16 MHz)

RCC->CFGR = 0x00000000
  └─ SWS (Clock Source):           0x0 (HSI)
  └─ PLLON (PLL Enable):           0 (DISABLED)
```

### AFTER (Gọi SystemClock_Config() + Clock_Init())
```
Current Clock Source: PLL (100 MHz) ✅
HCLK (AHB):         100000000 Hz (100 MHz)
PCLK1 (APB1):       50000000 Hz (50 MHz)  [÷2 Prescaler]
PCLK2 (APB2):       100000000 Hz (100 MHz)  [÷1 Prescaler]
SystemCoreClock:    100000000 Hz (100 MHz)

RCC->CFGR = 0x0000XXXX
  └─ SWS (Clock Source):           0x2 (PLL)
  └─ PLLON (PLL Enable):           1 (ENABLED)
```

---

## ⚠️ Dấu Hiệu Nhận Biết Đang Dùng HSI 16MHz

### Dấu Hiệu 1: HAL_Delay Chạy Chậm
```c
uint32_t start = HAL_GetTick();
HAL_Delay(1000);  // Đợi 1 giây
uint32_t elapsed = HAL_GetTick() - start;

if (elapsed > 1000) {
    printf("⚠️  Delay chạy chậm! Có thể đang dùng HSI 16 MHz\n");
}
```

### Dấu Hiệu 2: UART Garbage Data
```
Nếu UART được cấu hình cho 50 MHz APB1 nhưng thực tế chỉ có 16 MHz:
→ Baud rate sai, nhận được garbage data
→ Dấu hiệu: Dòng chữ bị lỗi trên terminal
```

### Dấu Hiệu 3: Motor PWM Frequency Sai
```c
// Nếu PWM frequency không phải 50 Hz:
// - Motor sẽ quay sai tốc độ
// - ESC sẽ nhận tín hiệu sai

// Kiểm tra: Dùng oscilloscope hoặc frequency counter
// Period phải là 20ms (50 Hz)
```

### Dấu Hiệu 4: SysTick Chạy Chậm
```c
// SysTick interrupt liên tục báo thời gian sai
// Tất cả timing-related code chạy sai
```

---

## ✅ Cách Đảm Bảo Đang Dùng PLL 100MHz

```c
void Verify_PLL_Running(void)
{
    uint32_t clock_source = (RCC->CFGR & RCC_CFGR_SWS) >> 2;
    uint32_t pll_enabled = (RCC->CR & RCC_CR_PLLON) >> 24;
    uint32_t pll_ready = (RCC->CR & RCC_CR_PLLRDY) >> 25;
    uint32_t hclk = HAL_RCC_GetHCLKFreq();
    
    if (clock_source == 2 && pll_enabled && pll_ready && hclk == 100000000) {
        printf("✅ PLL 100 MHz is running correctly!\n");
        return;
    }
    
    printf("❌ ERROR: Not using PLL 100 MHz!\n");
    printf("  Clock Source: %ld (should be 2 for PLL)\n", clock_source);
    printf("  PLL Enabled: %ld\n", pll_enabled);
    printf("  PLL Ready: %ld\n", pll_ready);
    printf("  HCLK: %lu Hz (should be 100000000)\n", hclk);
}
```

---

**Version**: 1.0  
**Date**: 2026-06-15  
**Author**: STM32 Development Team
