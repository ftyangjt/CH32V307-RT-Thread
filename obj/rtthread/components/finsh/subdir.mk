################################################################################
# MRS Version: 2.1.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../rtthread/components/finsh/cmd.c \
../rtthread/components/finsh/msh.c \
../rtthread/components/finsh/msh_cmd.c \
../rtthread/components/finsh/msh_file.c \
../rtthread/components/finsh/shell.c \
../rtthread/components/finsh/symbol.c 

C_DEPS += \
./rtthread/components/finsh/cmd.d \
./rtthread/components/finsh/msh.d \
./rtthread/components/finsh/msh_cmd.d \
./rtthread/components/finsh/msh_file.d \
./rtthread/components/finsh/shell.d \
./rtthread/components/finsh/symbol.d 

OBJS += \
./rtthread/components/finsh/cmd.o \
./rtthread/components/finsh/msh.o \
./rtthread/components/finsh/msh_cmd.o \
./rtthread/components/finsh/msh_file.o \
./rtthread/components/finsh/shell.o \
./rtthread/components/finsh/symbol.o 



# Each subdirectory must supply rules for building sources it contributes
rtthread/components/finsh/%.o: ../rtthread/components/finsh/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/Debug" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/Core" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/User" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/Peripheral/inc" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/drivers" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/include" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/include/libc" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/libcpu/risc-v" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/libcpu/risc-v/common" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/src" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/components/drivers/include" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/components/drivers/misc" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/components/drivers/serial" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/components/finsh" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
