
# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/dfu_mal.c \
../src/flash_if.c \
../src/main.c \
../src/spi_if.c \
../src/stm32_it.c \
../src/usb_desc.c \
../src/usb_istr.c \
../src/usb_prop.c 

OBJS += \
./src/dfu_mal.o \
./src/flash_if.o \
./src/main.o \
./src/spi_if.o \
./src/stm32_it.o \
./src/usb_desc.o \
./src/usb_istr.o \
./src/usb_prop.o 

C_DEPS += \
./src/dfu_mal.d \
./src/flash_if.d \
./src/main.d \
./src/spi_if.d \
./src/stm32_it.d \
./src/usb_desc.d \
./src/usb_istr.d \
./src/usb_prop.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Sourcery Windows GCC C Compiler'
	arm-none-eabi-gcc -DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD -I"../../core-common-lib/CMSIS/Include" -I"../../core-common-lib/CMSIS/Device/ST/STM32F10x/Include" -I"../../core-common-lib/STM32F10x_StdPeriph_Driver/inc" -I"../../core-common-lib/STM32_USB-FS-Device_Driver/inc" -I"../../core-common-lib/CC3000_Host_Driver" -I"../../core-common-lib/SPARK_Firmware_Driver/inc" -I"../inc" -Os -ffunction-sections -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


