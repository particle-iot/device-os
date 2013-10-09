
# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../STM32_USB-FS-Device_Driver/src/usb_core.c \
../STM32_USB-FS-Device_Driver/src/usb_init.c \
../STM32_USB-FS-Device_Driver/src/usb_int.c \
../STM32_USB-FS-Device_Driver/src/usb_mem.c \
../STM32_USB-FS-Device_Driver/src/usb_regs.c \
../STM32_USB-FS-Device_Driver/src/usb_sil.c 

OBJS += \
./STM32_USB-FS-Device_Driver/src/usb_core.o \
./STM32_USB-FS-Device_Driver/src/usb_init.o \
./STM32_USB-FS-Device_Driver/src/usb_int.o \
./STM32_USB-FS-Device_Driver/src/usb_mem.o \
./STM32_USB-FS-Device_Driver/src/usb_regs.o \
./STM32_USB-FS-Device_Driver/src/usb_sil.o 

C_DEPS += \
./STM32_USB-FS-Device_Driver/src/usb_core.d \
./STM32_USB-FS-Device_Driver/src/usb_init.d \
./STM32_USB-FS-Device_Driver/src/usb_int.d \
./STM32_USB-FS-Device_Driver/src/usb_mem.d \
./STM32_USB-FS-Device_Driver/src/usb_regs.d \
./STM32_USB-FS-Device_Driver/src/usb_sil.d 


# Each subdirectory must supply rules for building sources it contributes
STM32_USB-FS-Device_Driver/src/%.o: ../STM32_USB-FS-Device_Driver/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM GCC C Compiler'
	arm-none-eabi-gcc -DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD -I"../CMSIS/Include" -I"../CMSIS/Device/ST/STM32F10x/Include" -I"../STM32F10x_StdPeriph_Driver/inc" -I"../STM32_USB-FS-Device_Driver/inc" -I"../CC3000_Host_Driver" -I"../SPARK_Firmware_Driver/inc" -Os -ffunction-sections -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


