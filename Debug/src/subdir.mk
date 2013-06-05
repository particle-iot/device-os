################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/dfu_mal.c \
../src/flash_if.c \
../src/hw_config.c \
../src/main.c \
../src/spi_if.c \
../src/sst25vf_spi.c \
../src/stm32_it.c \
../src/system_stm32f10x.c \
../src/usb_desc.c \
../src/usb_istr.c \
../src/usb_prop.c \
../src/usb_pwr.c 

OBJS += \
./src/dfu_mal.o \
./src/flash_if.o \
./src/hw_config.o \
./src/main.o \
./src/spi_if.o \
./src/sst25vf_spi.o \
./src/stm32_it.o \
./src/system_stm32f10x.o \
./src/usb_desc.o \
./src/usb_istr.o \
./src/usb_prop.o \
./src/usb_pwr.o 

C_DEPS += \
./src/dfu_mal.d \
./src/flash_if.d \
./src/hw_config.d \
./src/main.d \
./src/spi_if.d \
./src/sst25vf_spi.d \
./src/stm32_it.d \
./src/system_stm32f10x.d \
./src/usb_desc.d \
./src/usb_istr.d \
./src/usb_prop.d \
./src/usb_pwr.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Sourcery Mac OS X GCC C Compiler'
	arm-none-eabi-gcc -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER -I"/Users/zac/code/spark/usb-dfu/libraries/CMSIS/Include" -I"/Users/zac/code/spark/usb-dfu/libraries/CMSIS/Device/ST/STM32F10x/Include" -I"/Users/zac/code/spark/usb-dfu/libraries/STM32F10x_StdPeriph_Driver/inc" -I"/Users/zac/code/spark/usb-dfu/libraries/STM32_USB-FS-Device_Driver/inc" -I"/Users/zac/code/spark/usb-dfu/inc" -Os -ffunction-sections -Wall -Wa,-adhlns="$@.lst" -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


