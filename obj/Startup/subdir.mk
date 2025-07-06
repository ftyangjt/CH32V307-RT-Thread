################################################################################
# MRS Version: 2.1.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
../Startup/startup_ch32v30x_D8C.S 

S_UPPER_DEPS += \
./Startup/startup_ch32v30x_D8C.d 

OBJS += \
./Startup/startup_ch32v30x_D8C.o 



# Each subdirectory must supply rules for building sources it contributes
Startup/%.o: ../Startup/%.S
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -x assembler-with-cpp -I"c:/Users/wydnm/Desktop/GIT/NEW/CH32V307-RT-Thread/Startup" -I"c:/Users/wydnm/Desktop/GIT/NEW/CH32V307-RT-Thread/drivers" -I"c:/Users/wydnm/Desktop/GIT/NEW/CH32V307-RT-Thread/rtthread/include" -I"c:/Users/wydnm/Desktop/GIT/NEW/CH32V307-RT-Thread/rtthread/include/libc" -I"c:/Users/wydnm/Desktop/GIT/NEW/CH32V307-RT-Thread/rtthread/libcpu" -I"c:/Users/wydnm/Desktop/GIT/NEW/CH32V307-RT-Thread/rtthread/libcpu/risc-v/common" -I"c:/Users/wydnm/Desktop/GIT/NEW/CH32V307-RT-Thread/rtthread/src" -I"c:/Users/wydnm/Desktop/GIT/NEW/CH32V307-RT-Thread/rtthread/components/drivers/include" -I"c:/Users/wydnm/Desktop/GIT/NEW/CH32V307-RT-Thread/rtthread/components/drivers/misc" -I"c:/Users/wydnm/Desktop/GIT/NEW/CH32V307-RT-Thread/rtthread/components/drivers/serial" -I"c:/Users/wydnm/Desktop/GIT/NEW/CH32V307-RT-Thread/rtthread/components/finsh" -I"c:/Users/wydnm/Desktop/GIT/NEW/CH32V307-RT-Thread/rtthread" -I"c:/Users/wydnm/Desktop/GIT/NEW/CH32V307-RT-Thread/Middlewares" -I"c:/Users/wydnm/Desktop/GIT/NEW/CH32V307-RT-Thread/Middlewares/FATFS" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
