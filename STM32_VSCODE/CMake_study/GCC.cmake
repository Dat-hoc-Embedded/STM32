set(CMAKE_SYSTEM_NAME Generic)   
set(CMAKE_SYSTEM_PROCESSOR ARM)  # build cho target embedded (ARM MCU) -> Báo cho CMake đây là "cross compile"

set(ARM_TOOLCHAIN_DIR "E:/CODE/STM32_IDE/STM32_VSCODE/CMake_study/Gcc_Compiler/bin")   # nơi chứa gcc-arm
set(BINUTILS_PATH ${ARM_TOOLCHAIN_DIR}/../lib)   # 

set(TOOLCHAIN_PREFIX ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-) 

set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc.exe)        # arm-none-eabi-gcc.exe
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER}gcc.exe)        # ASM dùng chung gcc.exe
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++.exe)      # arm-none-eabi-g++.exe

set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy CACHE INTERNAL "objcopy tool")   # objcopy → convert .elf → .bin/.hex
set(CMAKE_SIZE_UTIL ${TOOLCHAIN_PREFIX}size CACHE INTERNAL "size tool")       # size → xem size firmware

set(CMAKE_FIND_ROOT_PATH ${BINUTILS_PATH})          # CMake sẽ tìm library, header trong toolchain thay vì PC
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)        # program sẽ dùng của host (PC)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)         # dùng của toolchain 
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)         # dùng của toolchain

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)   # Bình thường CMake sẽ thử compile & chạy test program nhưng embedded nên chuyển sang chỉ compile

message(STATUS "ARM_TOOLCHAIN_DIR = ${ARM_TOOLCHAIN_DIR}")
message(STATUS "C compiler = ${CMAKE_C_COMPILER}")
message(STATUS "CXX compiler = ${CMAKE_CXX_COMPILER}")
message(STATUS "Current_source = ${CMAKE_CURRENT_SOURCE_DIR}")    # E:/CODE/STM32_IDE/STM32_VSCODE/CMake_study 