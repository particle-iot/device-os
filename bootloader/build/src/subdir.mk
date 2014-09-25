
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

PLATFORM=../../common/SPARK_Platform/
SERVICES=../../common/SPARK_Services/
HAL=../../common/SPARK_Hal/
PLATFORM_MCU=$(PLATFORM)MCU/STM32F1xx/
PLATFORM_CC3000=$(PLATFORM)WLAN/CC3000_Host_Driver

CFLAGS=-mcpu=cortex-m3 -mthumb -g3 -gdwarf-2
CFLAGS+=-Werror

# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Sourcery Windows GCC C Compiler'
	arm-none-eabi-gcc -DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD -I"../inc" -I"$(PLATFORM_MCU)CMSIS/Include" -I"$(PLATFORM_MCU)CMSIS/Device/ST/Include" -I"$(PLATFORM_MCU)STM32_StdPeriph_Driver/inc" -I"$(PLATFORM_MCU)STM32_USB_Device_Driver/inc" -I"$(PLATFORM_CC3000)CC3000_Host_Driver" -I"$(PLATFORM_MCU)SPARK_Firmware_Driver/inc" -I"$(SERVICES)/inc" -I"$(HAL)/inc" -Os -ffunction-sections -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" $(CFLAGS) -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


