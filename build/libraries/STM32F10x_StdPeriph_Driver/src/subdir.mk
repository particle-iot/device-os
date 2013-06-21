########################################
#   @author  Spark Application Team    #
#   @version V1.0.0                    #
#   @date    20-June-2013              #
########################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../libraries/STM32F10x_StdPeriph_Driver/src/misc.c \
../libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_bkp.c \
../libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_dma.c \
../libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_exti.c \
../libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_flash.c \
../libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c \
../libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_pwr.c \
../libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c \
../libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_spi.c \
../libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_tim.c 

OBJS += \
./libraries/STM32F10x_StdPeriph_Driver/src/misc.o \
./libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_bkp.o \
./libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_dma.o \
./libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_exti.o \
./libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_flash.o \
./libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.o \
./libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_pwr.o \
./libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.o \
./libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_spi.o \
./libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_tim.o 

C_DEPS += \
./libraries/STM32F10x_StdPeriph_Driver/src/misc.d \
./libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_bkp.d \
./libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_dma.d \
./libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_exti.d \
./libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_flash.d \
./libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.d \
./libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_pwr.d \
./libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.d \
./libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_spi.d \
./libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_tim.d 


# Each subdirectory must supply rules for building sources it contributes
libraries/STM32F10x_StdPeriph_Driver/src/%.o: ../libraries/STM32F10x_StdPeriph_Driver/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM GCC C Compiler'
	arm-none-eabi-gcc -DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD -I"../libraries/CMSIS/Include" -I"../libraries/CMSIS/Device/ST/STM32F10x/Include" -I"../libraries/STM32F10x_StdPeriph_Driver/inc" -I"../libraries/STM32_USB-FS-Device_Driver/inc" -I"../inc" -Os -ffunction-sections -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


