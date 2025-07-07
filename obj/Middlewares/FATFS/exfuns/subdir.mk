################################################################################
# MRS Version: 2.1.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/FATFS/exfuns/exfuns.c \
../Middlewares/FATFS/exfuns/fattester.c \
../Middlewares/FATFS/exfuns/myffunicode.c 

C_DEPS += \
./Middlewares/FATFS/exfuns/exfuns.d \
./Middlewares/FATFS/exfuns/fattester.d \
./Middlewares/FATFS/exfuns/myffunicode.d 

OBJS += \
./Middlewares/FATFS/exfuns/exfuns.o \
./Middlewares/FATFS/exfuns/fattester.o \
./Middlewares/FATFS/exfuns/myffunicode.o 



# Each subdirectory must supply rules for building sources it contributes
Middlewares/FATFS/exfuns/%.o: ../Middlewares/FATFS/exfuns/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/Debug" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/Core" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/User" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/Peripheral/inc" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/drivers" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/include" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/include/libc" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/libcpu/risc-v" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/libcpu/risc-v/common" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/src" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/components/drivers/include" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/components/drivers/misc" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/components/drivers/serial" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/components/finsh" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/Middlewares" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
