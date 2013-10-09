
# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/application.cpp \
../src/main.cpp \
../src/newlib_stubs.cpp \
../src/spark_utilities.cpp \
../src/spark_wiring.cpp \
../src/stm32_it.cpp \
../src/usb_desc.cpp \
../src/usb_endp.cpp \
../src/usb_istr.cpp \
../src/usb_prop.cpp 

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

CPP_DEPS += \
./src/application.d \
./src/main.d \
./src/newlib_stubs.d \
./src/spark_utilities.d \
./src/spark_wiring.d \
./src/stm32_it.d \
./src/usb_desc.d \
./src/usb_endp.d \
./src/usb_istr.d \
./src/usb_prop.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: ARM GCC C++ Compiler'
	arm-none-eabi-g++ -DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD -DDFU_BUILD_ENABLE -I"../../core-common-lib/CMSIS/Include" -I"../../core-common-lib/CMSIS/Device/ST/STM32F10x/Include" -I"../../core-common-lib/STM32F10x_StdPeriph_Driver/inc" -I"../../core-common-lib/STM32_USB-FS-Device_Driver/inc" -I"../../core-common-lib/CC3000_Host_Driver" -I"../../core-common-lib/SPARK_Firmware_Driver/inc" -I"../../core-communication-lib/lib/tropicssl/include" -I"../../core-communication-lib/src" -I"../inc" -Os -ffunction-sections -Wall -Werror -fno-exceptions -fno-rtti -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


