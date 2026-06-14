# STM32 Drone Control System

## 📋 Tổng Quan Dự Án

Đây là dự án **điều khiển drone** dựa trên microcontroller **STM32F411VETx**, được phát triển sử dụng **VS Code** và **STM32CubeMX**. Dự án cung cấp các module điều khiển động cơ, nhận tín hiệu từ remote control (RC), và tích hợp các cảm biến.

### Thông Số Kỹ Thuật Chính
- **Microcontroller**: STM32F411VETx (ARM Cortex-M4, 100MHz)
- **Flash Memory**: 512KB
- **RAM**: 128KB
- **IDE**: Visual Studio Code + STM32CubeMX
- **Build System**: CMake
- **Programming**: SWD (Serial Wire Debug) via ST-Link

---
### Peripherals



---

## 📁 Cấu Trúc Thư Mục

```
DRONE_VSCODE/
├── .vscode/                        # VS Code configuration
│   ├── c_cpp_properties.json       # IntelliSense & Include paths config
│   ├── extensions.json             # Recommended VS Code extensions
│   ├── launch.json                 # Debug & Flash configuration
│   ├── settings.json               # Workspace settings
│   └── tasks.json                  # Build & Flash automation tasks
│
├── Core/                           # Application code (CPU core layer)
│   ├── Inc/                        # Header files
│   │   ├── main.h                  # Main application header
│   │   ├── stm32f4xx_hal_conf.h    # HAL Driver configuration
│   │   └── stm32f4xx_it.h          # Interrupt handlers header
│   │
│   ├── Src/                        # Source files
│   │   ├── main.c                  # Main application entry point
│   │   ├── stm32f4xx_hal_msp.c     # HAL MSP (MCU Support Package)
│   │   ├── stm32f4xx_it.c          # Interrupt service routines (ISR)
│   │   ├── syscalls.c              # System calls for newlib
│   │   ├── sysmem.c                # Memory management
│   │   └── system_stm32f4xx.c      # System initialization
│   │
│   └── User/                       # User-defined modules
│       ├── board_config.h          # Board hardware configuration
│       ├── motor/                  # Motor control module
│       │   ├── motor.h
│       │   └── motor.c
│       ├── RC/                     # Remote Control receiver module
│       │   ├── receiver.h
│       │   └── receiver.c
│       ├── uart/                   # UART communication module
│       │   ├── uart.h
│       │   └── uart.c
│       └── Sensor/                 # Sensor module (placeholder)
│           └── README.md           # Sensor documentation
│
├── Drivers/                        # External driver libraries
│   ├── CMSIS/                      # ARM CMSIS (Cortex Microcontroller Software Interface Standard)
│   │   ├── Core/                   # CMSIS Core - ARM Cortex-M generic definitions
│   │   ├── Device/ST/STM32F4xx/    # STM32F4xx device definitions & startup
│   │   ├── DSP/                    # Digital Signal Processing library
│   │   └── NN/                     # Neural Network library
│   │
│   └── STM32F4xx_HAL_Driver/       # STM32 HAL Driver (Hardware Abstraction Layer)
│       ├── Inc/                    # HAL header files (GPIO, UART, SPI, etc.)
│       └── Src/                    # HAL source files (implementations)
│
├── build/                          # CMake build output folder (generated)
│   ├── Debug/                      # Debug build artifacts
│   └── CMakeLists.txt
│
├── cmake/                          # CMake configuration files
│   ├── gcc-arm-none-eabi.cmake     # ARM GCC toolchain configuration
│   └── stm32cubemx/                # STM32CubeMX CMake integration
│
├── CMakeLists.txt                  # Root CMake build configuration
├── CMakePresets.json               # CMake build presets
├── compile_commands.json           # Clang compiler database (for IDE)
├── STM32F411XX_FLASH.ld            # Linker script (memory layout)
├── startup_stm32f411xe.s           # Startup assembly code
├── TEST_VSCODE_Extension.ioc       # STM32CubeMX project file
└── README.md                        # This file

```

---

## 📌 Mô Tả Chi Tiết Từng Folder/File

### 1. **`.vscode/` - VS Code Configuration**
**Mục đích**: Cấu hình môi trường phát triển cho toàn bộ workspace

| File | Mục đích | Tại sao cần thiết |
|------|---------|------------------|
| `c_cpp_properties.json` | Cấu hình IntelliSense (autocomplete, linting) | Giúp VS Code hiểu cấu trúc dự án STM32, cung cấp autocomplete chính xác |
| `extensions.json` | Danh sách extensions khuyến nghị | Đảm bảo mọi người sử dụng cùng bộ công cụ |
| `launch.json` | Cấu hình debug & flash firmware | Cho phép debug trực tiếp trên chip & nạp chương trình qua SWD |
| `settings.json` | Cài đặt VS Code workspace-specific | Tùy chỉnh hành vi editor cho dự án |
| `tasks.json` | Định nghĩa tác vụ build/flash tự động | Tự động hóa quy trình build & flash |

---

### 2. **`Core/` - Application Layer**
**Mục đích**: Chứa mã ứng dụng chính, xử lý logic kinh doanh

#### `Core/Inc/` - Header Files
- **`main.h`**: Định nghĩa hằng số, hàm, các biến toàn cục của ứng dụng
- **`stm32f4xx_hal_conf.h`**: Cấu hình HAL Driver (bật/tắt module như GPIO, UART, Timer)
- **`stm32f4xx_it.h`**: Định nghĩa các interrupt handler

#### `Core/Src/` - Implementation
- **`main.c`**: Điểm vào chương trình (main function), khởi tạo hệ thống
- **`stm32f4xx_hal_msp.c`**: MSP (MCU Support Package) - cấu hình HAL ở mức thấp
- **`stm32f4xx_it.c`**: Xử lý các interrupt (timer, UART, GPIO events)
- **`system_stm32f4xx.c`**: Khởi tạo hệ thống, clock configuration
- **`syscalls.c`, `sysmem.c`**: Support cho C standard library (printf, malloc, etc.)

#### `Core/User/` - Custom Modules
**Chứa các module do người dùng viết**, không được generate tự động từ CubeMX

- **`board_config.h`**: Cấu hình phần cứng (pinout, clock, settings)
- **`motor/`**: Module điều khiển động cơ
  - `motor.h`: API điều khiển động cơ (start, stop, speed)
  - `motor.c`: Thực hiện logic PWM, điều chỉnh tốc độ
  - **Tại sao cần thiết**: Drone cần điều khiển 4 động cơ để bay
  
- **`RC/`**: Module nhận tín hiệu remote control
  - `receiver.h/c`: Giải mã tín hiệu RC (PPM, PWM, SBUS)
  - **Tại sao cần thiết**: Người dùng điều khiển drone qua remote
  
- **`uart/`**: Module giao tiếp UART
  - `uart.h/c`: Gửi/nhận dữ liệu qua serial
  - **Tại sao cần thiết**: Debug log, telemetry, giao tiếp với máy tính

- **`Sensor/`**: Placeholder cho cảm biến (IMU, barometer)
  - **Tại sao cần thiết**: Drone cần cảm biến để ổn định, định vị

---

### 3. **`Drivers/` - HAL & Low-Level Drivers**
**Mục đích**: Chứa thư viện driver do nhà sản xuất (ST) cung cấp

#### `Drivers/CMSIS/`
**CMSIS = Cortex Microcontroller Software Interface Standard** (chuẩn ARM toàn cầu)

- **`Core/`**: Định nghĩa chung cho tất cả ARM Cortex-M
- **`Device/ST/STM32F4xx/`**: Định nghĩa thanh ghi & địa chỉ bộ nhớ của STM32F4xx
  - **Tại sao cần thiết**: Để truy cập thanh ghi GPIO, UART, Timer một cách chính xác
- **`DSP/`**: Digital Signal Processing library (FFT, filters) - tùy chọn
- **`NN/`**: Neural Network library - tùy chọn

#### `Drivers/STM32F4xx_HAL_Driver/`
**HAL = Hardware Abstraction Layer** - cung cấp API dễ sử dụng

- **`Inc/`**: Header của HAL (stm32f4xx_hal_gpio.h, stm32f4xx_hal_uart.h, v.v.)
- **`Src/`**: Thực hiện HAL functions
- **Tại sao cần thiết**: 
  - Không cần viết driver từ đầu
  - HAL cung cấp functions cao cấp: `HAL_GPIO_WritePin()`, `HAL_UART_Transmit()`, v.v.
  - Giảm thiểu lỗi & tăng khả năng di chuyển code

---

### 4. **`cmake/` - Build Configuration**
**Mục đích**: Cấu hình build system CMake

- **`gcc-arm-none-eabi.cmake`**: Cấu hình ARM GCC toolchain (compiler, linker)
- **`stm32cubemx/`**: Cấu hình CMake cho STM32CubeMX
- **Tại sao cần thiết**: CMake biết cách biên dịch code cho ARM MCU

---

### 5. **`build/` - Build Output (Generated)**
**Mục đích**: Chứa kết quả build (object files, executable)

```
build/
└── Debug/
    ├── DRONE_VSCODE.elf      # Executable file (để flash vào chip)
    ├── DRONE_VSCODE.hex      # Intel HEX format
    ├── DRONE_VSCODE.bin      # Binary format
    └── CMakeFiles/           # Temporary build files
```

- **Tại sao cần thiết**: Folder này được generate tự động, chứa firmware cuối cùng để flash vào STM32

---

### 6. **CMake & Build Files**

| File | Mục đích | Tại sao cần thiết |
|------|---------|------------------|
| `CMakeLists.txt` | Định nghĩa cách build dự án | Chỉ thị CMake biên dịch những file nào, thứ tự, flags compiler |
| `CMakePresets.json` | Cấu hình build preset (Debug/Release) | Lưu trữ cài đặt build mặc định |
| `cmake_install.cmake` | Script cài đặt/triển khai sau build | Generated file: chứa hướng dẫn cài đặt file (copy .elf/.hex/.bin, v.v.) sau khi build xong. Được generate tự động từ `CMakeLists.txt` |
| `compile_commands.json` | Clang compiler database | IDE sử dụng để tìm lỗi nhanh hơn |

#### Chi tiết về `cmake_install.cmake`:
- **Loại file**: **Generated** - Được tự động sinh ra bởi CMake, không cần chỉnh sửa thủ công
- **Vị trí**: Xuất hiện ở cả:
  - Root level (`cmake_install.cmake`) - File chính
  - `build/Debug/cmake_install.cmake` - File cho build configuration cụ thể
- **Nội dung**: Chứa các lệnh CMake để **cài đặt (install)** các file sau build:
  - Copy executable (`.elf`, `.hex`, `.bin`) đến thư mục chỉ định
  - Copy header files, library files nếu cần
  - Thực hiện các tác vụ post-build
- **Sử dụng**: Thường được gọi bởi lệnh `cmake --install` hoặc `make install`
  ```bash
  cmake --build build/Debug
  cmake --install build/Debug --prefix install_dir
  ```
- **Tại sao cần thiết**: 
  - Tự động hóa việc triển khai firmware (không cần copy file thủ công)
  - Đảm bảo firmware được đặt đúng vị trí
  - Hỗ trợ các quy trình CI/CD tự động
- **Có thể xóa?** ❌ **Không** - Nếu xóa, phải chạy lại CMake configure để regenerate

---

### 7. **Linker Script & Startup**

| File | Mục đích | Tại sao cần thiết |
|------|---------|------------------|
| `STM32F411XX_FLASH.ld` | Linker script (bố cục bộ nhớ) | Chỉ thị linker đặt code ở đâu trong flash, data ở đâu trong RAM |
| `startup_stm32f411xe.s` | Assembly startup code | Khởi động hệ thống trước khi gọi main() |

---

### 8. **Project Files**

| File | Mục đích |
|------|---------|
| `TEST_VSCODE_Extension.ioc` | STM32CubeMX project file (giao diện đồ họa để cấu hình) |

---

## 🛠️ Yêu Cầu Hệ Thống

### Phần Mềm Bắt Buộc
1. **Visual Studio Code** (v1.80+)
2. **CMake** (v3.20+)
3. **ARM GCC Toolchain** (`arm-none-eabi-gcc`)
4. **STM32CubeMX** (cấu hình hardware)
5. **STM32_Programmer_CLI** (flash firmware)

### Phần Cứng
1. **STM32F411VETx** Microcontroller Board
2. **ST-Link v2** (debugger/programmer)
3. **USB Cable** (kết nối ST-Link)

### Extensions VS Code (Khuyến Nghị)
```json
- ms-vscode.cpptools              // C/C++ IntelliSense
- ms-vscode.cmake-tools           // CMake integration
- marus25.cortex-debug            // ARM Cortex-M debugging
- dan-c-underwood.arm             // ARM Assembly highlighting
- zixuanwang.linkerscript         // Linker script highlighting
```

---

## 📦 Setup & Cài Đặt

### 1. Clone hoặc mở project
```bash
cd DRONE_VSCODE
```

### 2. Cài đặt VS Code Extensions
```bash
# Mở project trong VS Code, nó sẽ gợi ý cài đặt các extensions
# Hoặc cài thủ công theo danh sách trong .vscode/extensions.json
```

### 3. Kiểm tra ARM Toolchain
```bash
arm-none-eabi-gcc --version
```

---

## 🚀 Build & Flash

### Build Project
```bash
# Dùng VS Code CMake extension
Ctrl+Shift+P → CMake: Build

# Hoặc dùng task
Ctrl+Shift+P → Tasks: Run Task → CMake: clean rebuild
```

### Flash to STM32
```bash
# Cách 1: Sử dụng task
Ctrl+Shift+P → Tasks: Run Task → CubeProg: Flash project (SWD)

# Cách 2: Build + Flash tự động
Ctrl+Shift+P → Tasks: Run Task → Build + Flash
```

### Debug
```bash
# F5 hoặc Ctrl+Shift+D
# Chọn "Build & Debug Microcontroller - ST-Link"
```

---

## 📝 File/Folder Necessity Matrix

| Folder/File | Bắt buộc? | Có thể xóa? | Lý do |
|-------------|----------|-----------|-------|
| `.vscode/` | ✅ Có | ❌ Không | Cấu hình VS Code, nếu xóa phải setup lại |
| `Core/` | ✅ Có | ❌ Không | Mã ứng dụng chính, không có nó dự án không chạy |
| `Drivers/` | ✅ Có | ❌ Không | HAL driver, cần để tương tác với phần cứng |
| `cmake/` | ✅ Có | ❌ Không | Build configuration, CMake cần nó để biên dịch |
| `build/` | ❌ Không | ✅ Có | Output build, có thể xóa & rebuild lại |
| `CMakeLists.txt` | ✅ Có | ❌ Không | Hướng dẫn build, nếu xóa không thể build |
| `STM32F411XX_FLASH.ld` | ✅ Có | ❌ Không | Bộ nhớ layout, sai → lỗi flash |
| `startup_stm32f411xe.s` | ✅ Có | ❌ Không | Khởi động hệ thống |

---

## 🔗 Tài Liệu Tham Khảo

- [STM32F411 Datasheet](https://www.st.com/resource/en/datasheet/stm32f411ve.pdf)
- [STM32 HAL Documentation](https://www.st.com/resource/en/user_manual/dm00135183-stm32f4-series-arm-based-32-bit-mcus-stmicroelectronics.pdf)
- [CMSIS Documentation](https://arm-software.github.io/CMSIS_5/General/html/index.html)
- [CMake Documentation](https://cmake.org/cmake/help/latest/)

---

## 💡 Ghi Chú Quan Trọng

1. **Không chỉnh sửa thủ công** các file generated từ CubeMX (`Core/Src/`, HAL files)
2. **Luôn sử dụng** `Core/User/` để thêm code tùy chỉnh
3. **Kiểm tra** flash/RAM size trước khi thêm tính năng mới
4. **Sử dụng** debug session để track lỗi runtime

---

**Created**: 2026-06-14  
**Last Updated**: 2026-06-14  
**Author**: STM32 Development Team

