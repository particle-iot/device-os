
# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/application.c \
../src/main.c \
../src/newlib_stubs.c \
../src/spark_wiring.c \
../src/stm32_it.c \
../src/usb_desc.c \
../src/usb_endp.c \
../src/usb_istr.c \
../src/usb_prop.c 

OBJS += \
./src/application.o \
./src/main.o \
./src/newlib_stubs.o \
./src/spark_utilities.o \
./src/spark_wiring.o \
./src/stm32_it.o \
./src/usb_desc.o \
./src/usb_endp.o \
./src/usb_istr.o \
./src/usb_prop.o 

C_DEPS += \
./src/application.d \
./src/main.d \
./src/newlib_stubs.d \
./src/spark_wiring.d \
./src/stm32_it.d \
./src/usb_desc.d \
./src/usb_endp.d \
./src/usb_istr.d \
./src/usb_prop.d 

C++_SRCS += \
../src/spark_utilities.cpp

C++_DEPS += \
./src/spark_utilities.d


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM GCC C Compiler'
	arm-none-eabi-gcc -DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD -DDFU_BUILD_ENABLE -I"../../core-common-lib/CMSIS/Include" -I"../../core-common-lib/CMSIS/Device/ST/STM32F10x/Include" -I"../../core-common-lib/STM32F10x_StdPeriph_Driver/inc" -I"../../core-common-lib/STM32_USB-FS-Device_Driver/inc" -I"../../core-common-lib/CC3000_Host_Driver" -I"../../core-common-lib/SPARK_Firmware_Driver/inc" -I"../../core-communication-lib/lib/tropicssl/include" -I"../../core-communication-lib/src" -I"../inc" -Os -ffunction-sections -Wall -Werror -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: ARM GCC C++ Compiler'
	arm-none-eabi-g++ -DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD -DDFU_BUILD_ENABLE -I"../../core-common-lib/CMSIS/Include" -I"../../core-common-lib/CMSIS/Device/ST/STM32F10x/Include" -I"../../core-common-lib/STM32F10x_StdPeriph_Driver/inc" -I"../../core-common-lib/STM32_USB-FS-Device_Driver/inc" -I"../../core-common-lib/CC3000_Host_Driver" -I"../../core-common-lib/SPARK_Firmware_Driver/inc" -I"../../core-communication-lib/lib/tropicssl/include" -I"../../core-communication-lib/src" -I"../inc" -Os -ffunction-sections -Wall -Werror -fno-exceptions -fno-rtti -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


