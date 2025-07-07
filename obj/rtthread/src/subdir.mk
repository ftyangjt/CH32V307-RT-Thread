################################################################################
# MRS Version: 2.1.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../rtthread/src/clock.c \
../rtthread/src/components.c \
../rtthread/src/cpu.c \
../rtthread/src/device.c \
../rtthread/src/idle.c \
../rtthread/src/ipc.c \
../rtthread/src/irq.c \
../rtthread/src/kservice.c \
../rtthread/src/mem.c \
../rtthread/src/memheap.c \
../rtthread/src/mempool.c \
../rtthread/src/object.c \
../rtthread/src/scheduler.c \
../rtthread/src/slab.c \
../rtthread/src/thread.c \
../rtthread/src/timer.c 

C_DEPS += \
./rtthread/src/clock.d \
./rtthread/src/components.d \
./rtthread/src/cpu.d \
./rtthread/src/device.d \
./rtthread/src/idle.d \
./rtthread/src/ipc.d \
./rtthread/src/irq.d \
./rtthread/src/kservice.d \
./rtthread/src/mem.d \
./rtthread/src/memheap.d \
./rtthread/src/mempool.d \
./rtthread/src/object.d \
./rtthread/src/scheduler.d \
./rtthread/src/slab.d \
./rtthread/src/thread.d \
./rtthread/src/timer.d 

OBJS += \
./rtthread/src/clock.o \
./rtthread/src/components.o \
./rtthread/src/cpu.o \
./rtthread/src/device.o \
./rtthread/src/idle.o \
./rtthread/src/ipc.o \
./rtthread/src/irq.o \
./rtthread/src/kservice.o \
./rtthread/src/mem.o \
./rtthread/src/memheap.o \
./rtthread/src/mempool.o \
./rtthread/src/object.o \
./rtthread/src/scheduler.o \
./rtthread/src/slab.o \
./rtthread/src/thread.o \
./rtthread/src/timer.o 



# Each subdirectory must supply rules for building sources it contributes
rtthread/src/%.o: ../rtthread/src/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/Debug" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/Core" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/User" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/Peripheral/inc" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/drivers" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/include" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/include/libc" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/libcpu/risc-v" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/libcpu/risc-v/common" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/src" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/components/drivers/include" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/components/drivers/misc" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/components/drivers/serial" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/components/finsh" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/Middlewares" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
