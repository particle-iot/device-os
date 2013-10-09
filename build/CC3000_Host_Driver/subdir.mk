
# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../CC3000_Host_Driver/cc3000_common.c \
../CC3000_Host_Driver/evnt_handler.c \
../CC3000_Host_Driver/hci.c \
../CC3000_Host_Driver/netapp.c \
../CC3000_Host_Driver/nvmem.c \
../CC3000_Host_Driver/security.c \
../CC3000_Host_Driver/socket.c \
../CC3000_Host_Driver/wlan.c 

OBJS += \
./CC3000_Host_Driver/cc3000_common.o \
./CC3000_Host_Driver/evnt_handler.o \
./CC3000_Host_Driver/hci.o \
./CC3000_Host_Driver/netapp.o \
./CC3000_Host_Driver/nvmem.o \
./CC3000_Host_Driver/security.o \
./CC3000_Host_Driver/socket.o \
./CC3000_Host_Driver/wlan.o 

C_DEPS += \
./CC3000_Host_Driver/cc3000_common.d \
./CC3000_Host_Driver/evnt_handler.d \
./CC3000_Host_Driver/hci.d \
./CC3000_Host_Driver/netapp.d \
./CC3000_Host_Driver/nvmem.d \
./CC3000_Host_Driver/security.d \
./CC3000_Host_Driver/socket.d \
./CC3000_Host_Driver/wlan.d 


# Each subdirectory must supply rules for building sources it contributes
CC3000_Host_Driver/%.o: ../CC3000_Host_Driver/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM GCC C Compiler'
	arm-none-eabi-gcc -DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD -I"../CMSIS/Include" -I"../CMSIS/Device/ST/STM32F10x/Include" -I"../STM32F10x_StdPeriph_Driver/inc" -I"../STM32_USB-FS-Device_Driver/inc" -I"../CC3000_Host_Driver" -I"../SPARK_Firmware_Driver/inc" -Os -ffunction-sections -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


