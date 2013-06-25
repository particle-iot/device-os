########################################
#   @author  Spark Application Team    #
#   @version V1.0.0                    #
#   @date    20-June-2013              #
########################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/application.c \
../src/cc3000_spi.c \
../src/hw_config.c \
../src/main.c \
../src/spark_utilities.c \
../src/spark_wiring.c \
../src/sst25vf_spi.c \
../src/stm32_it.c \
../src/system_stm32f10x.c \
../src/usb_desc.c \
../src/usb_endp.c \
../src/usb_istr.c \
../src/usb_prop.c \
../src/usb_pwr.c 

OBJS += \
./src/application.o \
./src/cc3000_spi.o \
./src/hw_config.o \
./src/main.o \
./src/spark_utilities.o \
./src/spark_wiring.o \
./src/sst25vf_spi.o \
./src/stm32_it.o \
./src/system_stm32f10x.o \
./src/usb_desc.o \
./src/usb_endp.o \
./src/usb_istr.o \
./src/usb_prop.o \
./src/usb_pwr.o 

C_DEPS += \
./src/application.d \
./src/cc3000_spi.d \
./src/hw_config.d \
./src/main.d \
./src/spark_utilities.d \
./src/spark_wiring.d \
./src/sst25vf_spi.d \
./src/stm32_it.d \
./src/system_stm32f10x.d \
./src/usb_desc.d \
./src/usb_endp.d \
./src/usb_istr.d \
./src/usb_prop.d \
./src/usb_pwr.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	arm-none-eabi-gcc -DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD -I"../libraries/CMSIS/Include" -I"../libraries/CMSIS/Device/ST/STM32F10x/Include" -I"../libraries/STM32F10x_StdPeriph_Driver/inc" -I"../libraries/STM32_USB-FS-Device_Driver/inc" -I"../libraries/CC3000_Host_Driver" -I"../inc" -Os -ffunction-sections -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


