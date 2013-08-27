
# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../SPARK_Firmware_Driver/src/cc3000_spi.c \
../SPARK_Firmware_Driver/src/hw_config.c \
../SPARK_Firmware_Driver/src/sst25vf_spi.c \
../SPARK_Firmware_Driver/src/system_stm32f10x.c \
../SPARK_Firmware_Driver/src/usb_pwr.c 

OBJS += \
./SPARK_Firmware_Driver/src/cc3000_spi.o \
./SPARK_Firmware_Driver/src/hw_config.o \
./SPARK_Firmware_Driver/src/sst25vf_spi.o \
./SPARK_Firmware_Driver/src/system_stm32f10x.o \
./SPARK_Firmware_Driver/src/usb_pwr.o 

C_DEPS += \
./SPARK_Firmware_Driver/src/cc3000_spi.d \
./SPARK_Firmware_Driver/src/hw_config.d \
./SPARK_Firmware_Driver/src/sst25vf_spi.d \
./SPARK_Firmware_Driver/src/system_stm32f10x.d \
./SPARK_Firmware_Driver/src/usb_pwr.d 


# Each subdirectory must supply rules for building sources it contributes
SPARK_Firmware_Driver/src/%.o: ../SPARK_Firmware_Driver/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Sourcery Windows GCC C Compiler'
	arm-none-eabi-gcc -DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD -I"../CMSIS/Include" -I"../CMSIS/Device/ST/STM32F10x/Include" -I"../STM32F10x_StdPeriph_Driver/inc" -I"../STM32_USB-FS-Device_Driver/inc" -I"../CC3000_Host_Driver" -I"../SPARK_Firmware_Driver/inc" -Os -ffunction-sections -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


