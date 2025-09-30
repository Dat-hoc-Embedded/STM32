# ---- Phép gán biến -----
PROJ_NAME := sum

# ---- Phép cộng biến -----
# OBJ_FILES := output/main.o output/$(PROJ_NAME).o
## Hoặc dùng
OBJ_FILES := output/main.o 
OBJ_FILES += output/$(PROJ_NAME).o

output/main.o: source\main.c
	@gcc -I./include -c source\main.c -o output/main.o
output/$(PROJ_NAME).o: source\$(PROJ_NAME).c
	@gcc -I./include -c source\$(PROJ_NAME).c -o output/$(PROJ_NAME).o

.PHONY: build
build: $(OBJ_FILES)
	@gcc $(OBJ_FILES) -o output/main.exe
	@echo Build done!

.PHONY: run
run: build
	@output/main.exe

.PHONY: clean 
clean: 
	@rm ./output/*
	@echo "Clear All!"

print-%: 
	@echo $($(subst print-,,$@))
