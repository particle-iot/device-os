
# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../STM32F10x_StdPeriph_Driver/src/misc.c \
../STM32F10x_StdPeriph_Driver/src/stm32f10x_adc.c \
../STM32F10x_StdPeriph_Driver/src/stm32f10x_bkp.c \
../STM32F10x_StdPeriph_Driver/src/stm32f10x_crc.c \
../STM32F10x_StdPeriph_Driver/src/stm32f10x_dma.c \
../STM32F10x_StdPeriph_Driver/src/stm32f10x_exti.c \
../STM32F10x_StdPeriph_Driver/src/stm32f10x_flash.c \
../STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c \
../STM32F10x_StdPeriph_Driver/src/stm32f10x_i2c.c \
../STM32F10x_StdPeriph_Driver/src/stm32f10x_iwdg.c \
../STM32F10x_StdPeriph_Driver/src/stm32f10x_pwr.c \
../STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c \
../STM32F10x_StdPeriph_Driver/src/stm32f10x_rtc.c \
../STM32F10x_StdPeriph_Driver/src/stm32f10x_spi.c \
../STM32F10x_StdPeriph_Driver/src/stm32f10x_tim.c \
../STM32F10x_StdPeriph_Driver/src/stm32f10x_usart.c 

OBJS += \
./STM32F10x_StdPeriph_Driver/src/misc.o \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_adc.o \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_bkp.o \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_crc.o \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_dma.o \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_exti.o \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_flash.o \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.o \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_i2c.o \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_iwdg.o \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_pwr.o \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.o \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_rtc.o \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_spi.o \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_tim.o \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_usart.o 

C_DEPS += \
./STM32F10x_StdPeriph_Driver/src/misc.d \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_adc.d \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_bkp.d \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_crc.d \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_dma.d \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_exti.d \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_flash.d \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.d \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_i2c.d \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_iwdg.d \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_pwr.d \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.d \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_rtc.d \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_spi.d \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_tim.d \
./STM32F10x_StdPeriph_Driver/src/stm32f10x_usart.d 


# Each subdirectory must supply rules for building sources it contributes
STM32F10x_StdPeriph_Driver/src/%.o: ../STM32F10x_StdPeriph_Driver/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM GCC C Compiler'
	arm-none-eabi-gcc -DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD -I"../CMSIS/Include" -I"../CMSIS/Device/ST/STM32F10x/Include" -I"../STM32F10x_StdPeriph_Driver/inc" -I"../STM32_USB-FS-Device_Driver/inc" -I"../CC3000_Host_Driver" -I"../SPARK_Firmware_Driver/inc" -Os -ffunction-sections -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


