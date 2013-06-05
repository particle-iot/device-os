################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../libraries/CC3000_Host_Driver/cc3000_common.c \
../libraries/CC3000_Host_Driver/evnt_handler.c \
../libraries/CC3000_Host_Driver/hci.c \
../libraries/CC3000_Host_Driver/netapp.c \
../libraries/CC3000_Host_Driver/nvmem.c \
../libraries/CC3000_Host_Driver/os.c \
../libraries/CC3000_Host_Driver/security.c \
../libraries/CC3000_Host_Driver/socket.c \
../libraries/CC3000_Host_Driver/wlan.c 

OBJS += \
./libraries/CC3000_Host_Driver/cc3000_common.o \
./libraries/CC3000_Host_Driver/evnt_handler.o \
./libraries/CC3000_Host_Driver/hci.o \
./libraries/CC3000_Host_Driver/netapp.o \
./libraries/CC3000_Host_Driver/nvmem.o \
./libraries/CC3000_Host_Driver/os.o \
./libraries/CC3000_Host_Driver/security.o \
./libraries/CC3000_Host_Driver/socket.o \
./libraries/CC3000_Host_Driver/wlan.o 

C_DEPS += \
./libraries/CC3000_Host_Driver/cc3000_common.d \
./libraries/CC3000_Host_Driver/evnt_handler.d \
./libraries/CC3000_Host_Driver/hci.d \
./libraries/CC3000_Host_Driver/netapp.d \
./libraries/CC3000_Host_Driver/nvmem.d \
./libraries/CC3000_Host_Driver/os.d \
./libraries/CC3000_Host_Driver/security.d \
./libraries/CC3000_Host_Driver/socket.d \
./libraries/CC3000_Host_Driver/wlan.d 


# Each subdirectory must supply rules for building sources it contributes
libraries/CC3000_Host_Driver/%.o: ../libraries/CC3000_Host_Driver/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Sourcery Mac OS X GCC C Compiler'
	arm-none-eabi-gcc -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER -I"/Users/zac/code/spark/marvin/libraries/CMSIS/Include" -I"/Users/zac/code/spark/marvin/libraries/CMSIS/Device/ST/STM32F10x/Include" -I"/Users/zac/code/spark/marvin/libraries/STM32F10x_StdPeriph_Driver/inc" -I"/Users/zac/code/spark/marvin/libraries/STM32_USB-FS-Device_Driver/inc" -I"/Users/zac/code/spark/marvin/inc" -I"/Users/zac/code/spark/marvin/libraries/CC3000_Host_Driver" -O0 -ffunction-sections -Wall -Wa,-adhlns="$@.lst" -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


