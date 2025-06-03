################################################################################
# MRS Version: 2.1.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../rtthread/components/drivers/misc/pin.c 

C_DEPS += \
./rtthread/components/drivers/misc/pin.d 

OBJS += \
./rtthread/components/drivers/misc/pin.o 



# Each subdirectory must supply rules for building sources it contributes
rtthread/components/drivers/misc/%.o: ../rtthread/components/drivers/misc/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/Users/wydnm/mounriver-studio-projects/CH32V307-RT-Thread/Debug" -I"c:/Users/wydnm/mounriver-studio-projects/CH32V307-RT-Thread/Core" -I"c:/Users/wydnm/mounriver-studio-projects/CH32V307-RT-Thread/User" -I"c:/Users/wydnm/mounriver-studio-projects/CH32V307-RT-Thread/Peripheral/inc" -I"c:/Users/wydnm/mounriver-studio-projects/CH32V307-RT-Thread/drivers" -I"c:/Users/wydnm/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/include" -I"c:/Users/wydnm/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/include/libc" -I"c:/Users/wydnm/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/libcpu/risc-v" -I"c:/Users/wydnm/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/libcpu/risc-v/common" -I"c:/Users/wydnm/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/src" -I"c:/Users/wydnm/mounriver-studio-projects/CH32V307-RT-Thread/rtthread" -I"c:/Users/wydnm/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/components/drivers/include" -I"c:/Users/wydnm/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/components/drivers/misc" -I"c:/Users/wydnm/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/components/drivers/serial" -I"c:/Users/wydnm/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/components/finsh" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
