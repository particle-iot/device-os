########################################
#   @author  Spark Application Team    #
#   @version V1.0.0                    #
#   @date    20-June-2013              #
########################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../libraries/STM32_USB-FS-Device_Driver/src/usb_core.c \
../libraries/STM32_USB-FS-Device_Driver/src/usb_init.c \
../libraries/STM32_USB-FS-Device_Driver/src/usb_int.c \
../libraries/STM32_USB-FS-Device_Driver/src/usb_mem.c \
../libraries/STM32_USB-FS-Device_Driver/src/usb_regs.c \
../libraries/STM32_USB-FS-Device_Driver/src/usb_sil.c 

OBJS += \
./libraries/STM32_USB-FS-Device_Driver/src/usb_core.o \
./libraries/STM32_USB-FS-Device_Driver/src/usb_init.o \
./libraries/STM32_USB-FS-Device_Driver/src/usb_int.o \
./libraries/STM32_USB-FS-Device_Driver/src/usb_mem.o \
./libraries/STM32_USB-FS-Device_Driver/src/usb_regs.o \
./libraries/STM32_USB-FS-Device_Driver/src/usb_sil.o 

C_DEPS += \
./libraries/STM32_USB-FS-Device_Driver/src/usb_core.d \
./libraries/STM32_USB-FS-Device_Driver/src/usb_init.d \
./libraries/STM32_USB-FS-Device_Driver/src/usb_int.d \
./libraries/STM32_USB-FS-Device_Driver/src/usb_mem.d \
./libraries/STM32_USB-FS-Device_Driver/src/usb_regs.d \
./libraries/STM32_USB-FS-Device_Driver/src/usb_sil.d 


# Each subdirectory must supply rules for building sources it contributes
libraries/STM32_USB-FS-Device_Driver/src/%.o: ../libraries/STM32_USB-FS-Device_Driver/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM GCC C Compiler'
	arm-none-eabi-gcc -DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD -I"../libraries/CMSIS/Include" -I"../libraries/CMSIS/Device/ST/STM32F10x/Include" -I"../libraries/STM32F10x_StdPeriph_Driver/inc" -I"../libraries/STM32_USB-FS-Device_Driver/inc" -I"../inc" -Os -ffunction-sections -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


