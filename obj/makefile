################################################################################
# MRS Version: 2.1.0
# Automatically-generated file. Do not edit!
################################################################################
-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include rtthread/src/subdir.mk
-include rtthread/libcpu/risc-v/common/subdir.mk
-include rtthread/components/finsh/subdir.mk
-include rtthread/components/drivers/serial/subdir.mk
-include rtthread/components/drivers/misc/subdir.mk
-include rtthread/subdir.mk
-include drivers/SYSTEM/usart/subdir.mk
-include drivers/SYSTEM/sys/subdir.mk
-include drivers/SYSTEM/delay/subdir.mk
-include drivers/BSP/SPI/subdir.mk
-include drivers/BSP/SDIO/subdir.mk
-include drivers/BSP/NORFLASH/subdir.mk
-include drivers/BSP/LED/subdir.mk
-include drivers/BSP/LCD/subdir.mk
-include drivers/BSP/KEY/subdir.mk
-include drivers/BSP/ADC/subdir.mk
-include drivers/subdir.mk
-include User/ws2812b/subdir.mk
-include User/screen/subdir.mk
-include User/subdir.mk
-include Startup/subdir.mk
-include Peripheral/src/subdir.mk
-include Middlewares/USMART/subdir.mk
-include Middlewares/TEXT/subdir.mk
-include Middlewares/PICTURE/subdir.mk
-include Middlewares/MALLOC/subdir.mk
-include Middlewares/FATFS/source/subdir.mk
-include Middlewares/FATFS/exfuns/subdir.mk
-include Debug/subdir.mk
-include Core/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
CH32V307-RT-Thread.hex \

SECONDARY_LIST += \
CH32V307-RT-Thread.lst \

SECONDARY_SIZE += \
CH32V307-RT-Thread.siz \


# All Target
all: 
	$(MAKE) --no-print-directory main-build 

main-build: CH32V307-RT-Thread.elf secondary-outputs

# Tool invocations
CH32V307-RT-Thread.elf: $(OBJS) $(USER_OBJS)
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -T "c:/Users/wydnm/Desktop/GIT/NEW/CH32V307-RT-Thread/Ld/Link.ld" -nostartfiles -Xlinker --gc-sections -Xlinker --print-memory-usage -Wl,-Map,"CH32V307-RT-Thread.map" --specs=nano.specs --specs=nosys.specs -o "CH32V307-RT-Thread.elf" $(OBJS) $(USER_OBJS) $(LIBS)
CH32V307-RT-Thread.hex: CH32V307-RT-Thread.elf
	@	riscv-none-embed-objcopy -O ihex "CH32V307-RT-Thread.elf" "CH32V307-RT-Thread.hex"
CH32V307-RT-Thread.lst: CH32V307-RT-Thread.elf
	@	riscv-none-embed-objdump --all-headers --demangle --disassemble -M xw "CH32V307-RT-Thread.elf" > "CH32V307-RT-Thread.lst"
CH32V307-RT-Thread.siz: CH32V307-RT-Thread.elf
	riscv-none-embed-size --format=berkeley "CH32V307-RT-Thread.elf"

# Other Targets
clean:
	-$(RM) $(OBJS)$(SECONDARY_FLASH)$(SECONDARY_LIST)$(SECONDARY_SIZE)$(S_DEPS)$(S_UPPER_DEPS)$(ASM_DEPS)$(ASM_UPPER_DEPS)$(C_DEPS) CH32V307-RT-Thread.elf

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents

-include ../makefile.targets