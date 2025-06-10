################################################################################
# MRS Version: 2.1.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../drivers/CH32V30x_Driver/src/ch32v30x_adc.c \
../drivers/CH32V30x_Driver/src/ch32v30x_bkp.c \
../drivers/CH32V30x_Driver/src/ch32v30x_can.c \
../drivers/CH32V30x_Driver/src/ch32v30x_crc.c \
../drivers/CH32V30x_Driver/src/ch32v30x_dac.c \
../drivers/CH32V30x_Driver/src/ch32v30x_dbgmcu.c \
../drivers/CH32V30x_Driver/src/ch32v30x_dma.c \
../drivers/CH32V30x_Driver/src/ch32v30x_dvp.c \
../drivers/CH32V30x_Driver/src/ch32v30x_eth.c \
../drivers/CH32V30x_Driver/src/ch32v30x_exti.c \
../drivers/CH32V30x_Driver/src/ch32v30x_flash.c \
../drivers/CH32V30x_Driver/src/ch32v30x_fsmc.c \
../drivers/CH32V30x_Driver/src/ch32v30x_gpio.c \
../drivers/CH32V30x_Driver/src/ch32v30x_i2c.c \
../drivers/CH32V30x_Driver/src/ch32v30x_iwdg.c \
../drivers/CH32V30x_Driver/src/ch32v30x_misc.c \
../drivers/CH32V30x_Driver/src/ch32v30x_opa.c \
../drivers/CH32V30x_Driver/src/ch32v30x_pwr.c \
../drivers/CH32V30x_Driver/src/ch32v30x_rcc.c \
../drivers/CH32V30x_Driver/src/ch32v30x_rng.c \
../drivers/CH32V30x_Driver/src/ch32v30x_rtc.c \
../drivers/CH32V30x_Driver/src/ch32v30x_sdio.c \
../drivers/CH32V30x_Driver/src/ch32v30x_spi.c \
../drivers/CH32V30x_Driver/src/ch32v30x_tim.c \
../drivers/CH32V30x_Driver/src/ch32v30x_usart.c \
../drivers/CH32V30x_Driver/src/ch32v30x_wwdg.c 

C_DEPS += \
./drivers/CH32V30x_Driver/src/ch32v30x_adc.d \
./drivers/CH32V30x_Driver/src/ch32v30x_bkp.d \
./drivers/CH32V30x_Driver/src/ch32v30x_can.d \
./drivers/CH32V30x_Driver/src/ch32v30x_crc.d \
./drivers/CH32V30x_Driver/src/ch32v30x_dac.d \
./drivers/CH32V30x_Driver/src/ch32v30x_dbgmcu.d \
./drivers/CH32V30x_Driver/src/ch32v30x_dma.d \
./drivers/CH32V30x_Driver/src/ch32v30x_dvp.d \
./drivers/CH32V30x_Driver/src/ch32v30x_eth.d \
./drivers/CH32V30x_Driver/src/ch32v30x_exti.d \
./drivers/CH32V30x_Driver/src/ch32v30x_flash.d \
./drivers/CH32V30x_Driver/src/ch32v30x_fsmc.d \
./drivers/CH32V30x_Driver/src/ch32v30x_gpio.d \
./drivers/CH32V30x_Driver/src/ch32v30x_i2c.d \
./drivers/CH32V30x_Driver/src/ch32v30x_iwdg.d \
./drivers/CH32V30x_Driver/src/ch32v30x_misc.d \
./drivers/CH32V30x_Driver/src/ch32v30x_opa.d \
./drivers/CH32V30x_Driver/src/ch32v30x_pwr.d \
./drivers/CH32V30x_Driver/src/ch32v30x_rcc.d \
./drivers/CH32V30x_Driver/src/ch32v30x_rng.d \
./drivers/CH32V30x_Driver/src/ch32v30x_rtc.d \
./drivers/CH32V30x_Driver/src/ch32v30x_sdio.d \
./drivers/CH32V30x_Driver/src/ch32v30x_spi.d \
./drivers/CH32V30x_Driver/src/ch32v30x_tim.d \
./drivers/CH32V30x_Driver/src/ch32v30x_usart.d \
./drivers/CH32V30x_Driver/src/ch32v30x_wwdg.d 

OBJS += \
./drivers/CH32V30x_Driver/src/ch32v30x_adc.o \
./drivers/CH32V30x_Driver/src/ch32v30x_bkp.o \
./drivers/CH32V30x_Driver/src/ch32v30x_can.o \
./drivers/CH32V30x_Driver/src/ch32v30x_crc.o \
./drivers/CH32V30x_Driver/src/ch32v30x_dac.o \
./drivers/CH32V30x_Driver/src/ch32v30x_dbgmcu.o \
./drivers/CH32V30x_Driver/src/ch32v30x_dma.o \
./drivers/CH32V30x_Driver/src/ch32v30x_dvp.o \
./drivers/CH32V30x_Driver/src/ch32v30x_eth.o \
./drivers/CH32V30x_Driver/src/ch32v30x_exti.o \
./drivers/CH32V30x_Driver/src/ch32v30x_flash.o \
./drivers/CH32V30x_Driver/src/ch32v30x_fsmc.o \
./drivers/CH32V30x_Driver/src/ch32v30x_gpio.o \
./drivers/CH32V30x_Driver/src/ch32v30x_i2c.o \
./drivers/CH32V30x_Driver/src/ch32v30x_iwdg.o \
./drivers/CH32V30x_Driver/src/ch32v30x_misc.o \
./drivers/CH32V30x_Driver/src/ch32v30x_opa.o \
./drivers/CH32V30x_Driver/src/ch32v30x_pwr.o \
./drivers/CH32V30x_Driver/src/ch32v30x_rcc.o \
./drivers/CH32V30x_Driver/src/ch32v30x_rng.o \
./drivers/CH32V30x_Driver/src/ch32v30x_rtc.o \
./drivers/CH32V30x_Driver/src/ch32v30x_sdio.o \
./drivers/CH32V30x_Driver/src/ch32v30x_spi.o \
./drivers/CH32V30x_Driver/src/ch32v30x_tim.o \
./drivers/CH32V30x_Driver/src/ch32v30x_usart.o \
./drivers/CH32V30x_Driver/src/ch32v30x_wwdg.o 



# Each subdirectory must supply rules for building sources it contributes
drivers/CH32V30x_Driver/src/%.o: ../drivers/CH32V30x_Driver/src/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/Debug" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/Core" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/User" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/Peripheral/inc" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/drivers" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/include" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/include/libc" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/libcpu/risc-v" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/libcpu/risc-v/common" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/src" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/components/drivers/include" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/components/drivers/misc" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/components/drivers/serial" -I"c:/Users/lhxsy/mounriver-studio-projects/CH32V307-RT-Thread/rtthread/components/finsh" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
