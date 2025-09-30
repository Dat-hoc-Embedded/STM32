# CODE NOT OPTIMIZE
	#All: 
	#	arm-none-eabi-gcc -mcpu=cortex-m4 -std=gnu -I./include -c source\main.c -o output\main.o
	#	arm-none-eabi-gcc -mcpu=cortex-m4 -std=gnu -c source\GPIO.c -o output\GPIO.o
	#	arm-none-eabi-gcc -mcpu=cortex-m4 -std=gnu -c source\timer.c -o output\timer.o
	#	arm-none-eabi-gcc -mcpu=cortex-m4 -std=gnu -c -x assembler-with-cpp startup_stm32f411vetx.s -o output\startup.o
	#	arm-none-eabi-gcc -mcpu=cortex-m4 -std=gnu -T"STM32F411VETX_FLASH.ld" -Wl,-Map="output\output.map" -Wl,--gc-sections -static output\*.o -o firmware.elf
	#Load:
	#	STM32_Programmer_CLI -c port=SWD -e 0 -d firmware.elf -rst
#

# -------- Khai báo trình duyệt và đường dẫn đến LD_file & đường dẫn output ----------
GCC_ARM := arm-none-eabi-gcc 
CHIP := cortex-m4
LD_FILE := link/STM32F411VETX_FLASH.ld
START_FILE := start/startup_stm32f411vetx.s
PATH_OUTPUT := ./output

# -------- Lấy tất cả các Include Files ----------
INC_DIRS += include/GPIO \
			include/Timer 
INC_FILES = $(foreach dir, $(INC_DIRS), $(wildcard $(dir)/*))
# CC Opt for INC_DIRS: Thêm -I cho toàn bộ library
INC_DIRS_OPT := $(foreach var, $(INC_DIRS), -I"$(var)")

# -------- Lấy tất cả các Source Files ---------- 
SRC_DIRS := source 
SRC_FILES = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*))

# -------- Lấy tất cả các FIle Source Files và bỏ đi đường dẫn, rồi tạo ra thành file.o để làm yêu cầu cho rule ----------
OBJ_FILES := $(notdir $(SRC_FILES)) \
			$(notdir $(START_FILE))   # Lấy các file .c và .s
OBJ_FILES := $(foreach file, $(OBJ_FILES), $(basename $(file)).o) # Đổi hết đuôi .c -> .o
PATH_OBJ_FILES := $(foreach file, $(OBJ_FILES), $(PATH_OUTPUT)/$(file)) # thêm đường dẫn đến folder 

# -------- Các tập lệnh để tạo compiler & linker  -------------
CFLAGS := -mcpu=$(CHIP) -mthumb -std=gnu11 $(INC_DIRS_OPT) # compiler
ASFLAGS := -mcpu=$(CHIP) -std=gnu11 -x assembler-with-cpp # file startup
LDFLAGS := -mcpu=$(CHIP) -std=gnu11 -T $(LD_FILE) -Wl,-Map=output/program.map -Wl,--gc-sections -static # Linker

# -------- Các action --------
.PHONY: build
build: load/firmware.elf

# -------- Phần này sẽ giúp tự động tìm ra các file .c trong biến SRC_DIRS
vpath %.c $(SRC_DIRS)  # tìm file.c trong folder ./source
vpath %.s ./start # tìm file.s trong folder ./start

%.o:%.c
	@ $(GCC_ARM) $(CFLAGS) -c $^ -o $(PATH_OUTPUT)/$@ 
%.o:%.s
	@ $(GCC_ARM) $(ASFLAGS) -c $^ -o $(PATH_OUTPUT)/$@

load/firmware.elf: $(OBJ_FILES)
	@ $(GCC_ARM) $(LDFLAGS) $(PATH_OBJ_FILES) -o $@

load/firmware.hex: firmware.elf
	@ objcopy -O ihex $^ $@

.PHONY: load
load: load/firmware.elf 
	STM32_Programmer_CLI -c port=SWD -e 0 -d $< -rst

# -------------- Action bổ sung 
clean: # action để xóa 
	rm ./output/*
	rm ./load/*
 
print-%: # action để in ra -> debug
	@echo $($(subst print-,,$@))