########################################
#   @author  Spark Application Team    #
#   @version V1.0.0                    #
#   @date    20-June-2013              #
########################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../libraries/CC3000_Host_Driver/cc3000_common.c \
../libraries/CC3000_Host_Driver/evnt_handler.c \
../libraries/CC3000_Host_Driver/hci.c \
../libraries/CC3000_Host_Driver/netapp.c \
../libraries/CC3000_Host_Driver/nvmem.c \
../libraries/CC3000_Host_Driver/security.c \
../libraries/CC3000_Host_Driver/socket.c \
../libraries/CC3000_Host_Driver/wlan.c 

OBJS += \
./libraries/CC3000_Host_Driver/cc3000_common.o \
./libraries/CC3000_Host_Driver/evnt_handler.o \
./libraries/CC3000_Host_Driver/hci.o \
./libraries/CC3000_Host_Driver/netapp.o \
./libraries/CC3000_Host_Driver/nvmem.o \
./libraries/CC3000_Host_Driver/security.o \
./libraries/CC3000_Host_Driver/socket.o \
./libraries/CC3000_Host_Driver/wlan.o 

C_DEPS += \
./libraries/CC3000_Host_Driver/cc3000_common.d \
./libraries/CC3000_Host_Driver/evnt_handler.d \
./libraries/CC3000_Host_Driver/hci.d \
./libraries/CC3000_Host_Driver/netapp.d \
./libraries/CC3000_Host_Driver/nvmem.d \
./libraries/CC3000_Host_Driver/security.d \
./libraries/CC3000_Host_Driver/socket.d \
./libraries/CC3000_Host_Driver/wlan.d 


# Each subdirectory must supply rules for building sources it contributes
libraries/CC3000_Host_Driver/%.o: ../libraries/CC3000_Host_Driver/%.c
	arm-none-eabi-gcc -DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD -I"../libraries/CMSIS/Include" -I"../libraries/CMSIS/Device/ST/STM32F10x/Include" -I"../libraries/STM32F10x_StdPeriph_Driver/inc" -I"../libraries/STM32_USB-FS-Device_Driver/inc" -I"../libraries/CC3000_Host_Driver" -I"../inc" -Os -ffunction-sections -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o "$@" "$<"


