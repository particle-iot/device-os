
# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/application.cpp \
../src/main.cpp \
../src/newlib_stubs.cpp \
../src/spark_utilities.cpp \
../src/spark_wiring.cpp \
../src/spark_wiring_hardwareserial.cpp \
../src/spark_wiring_i2c.cpp \
../src/spark_wiring_interrupts.cpp \
../src/spark_wiring_ipaddress.cpp \
../src/spark_wiring_print.cpp \
../src/spark_wiring_spi.cpp \
../src/spark_wiring_stream.cpp \
../src/spark_wiring_string.cpp \
../src/spark_wiring_tcpclient.cpp \
../src/spark_wiring_tcpserver.cpp \
../src/spark_wiring_udp.cpp \
../src/spark_wiring_usartserial.cpp \
../src/spark_wiring_usbserial.cpp \
../src/spark_wlan.cpp \
../src/stm32_it.cpp \
../src/usb_desc.cpp \
../src/usb_endp.cpp \
../src/usb_istr.cpp \
../src/usb_prop.cpp \
../src/wifi_credentials_reader.cpp 

OBJS += \
./src/application.o \
./src/main.o \
./src/newlib_stubs.o \
./src/spark_utilities.o \
./src/spark_wiring.o \
./src/spark_wiring_hardwareserial.o \
./src/spark_wiring_i2c.o \
./src/spark_wiring_interrupts.o \
./src/spark_wiring_ipaddress.o \
./src/spark_wiring_print.o \
./src/spark_wiring_spi.o \
./src/spark_wiring_stream.o \
./src/spark_wiring_string.o \
./src/spark_wiring_tcpclient.o \
./src/spark_wiring_tcpserver.o \
./src/spark_wiring_udp.o \
./src/spark_wiring_usartserial.o \
./src/spark_wiring_usbserial.o \
./src/spark_wlan.o \
./src/stm32_it.o \
./src/usb_desc.o \
./src/usb_endp.o \
./src/usb_istr.o \
./src/usb_prop.o \
./src/wifi_credentials_reader.o 

CPP_DEPS += \
./src/application.d \
./src/main.d \
./src/newlib_stubs.d \
./src/spark_utilities.d \
./src/spark_wiring.d \
./src/spark_wiring_hardwareserial.d \
./src/spark_wiring_i2c.d \
./src/spark_wiring_interrupts.d \
./src/spark_wiring_ipaddress.d \
./src/spark_wiring_print.d \
./src/spark_wiring_spi.d \
./src/spark_wiring_stream.d \
./src/spark_wiring_string.d \
./src/spark_wiring_tcpclient.d \
./src/spark_wiring_tcpserver.d \
./src/spark_wiring_udp.d \
./src/spark_wiring_usartserial.d \
./src/spark_wiring_usbserial.d \
./src/spark_wlan.d \
./src/stm32_it.d \
./src/usb_desc.d \
./src/usb_endp.d \
./src/usb_istr.d \
./src/usb_prop.d \
./src/wifi_credentials_reader.d


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: ARM GCC C++ Compiler'
	arm-none-eabi-g++ -DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD -DDFU_BUILD_ENABLE -I"../../core-common-lib/CMSIS/Include" -I"../../core-common-lib/CMSIS/Device/ST/STM32F10x/Include" -I"../../core-common-lib/STM32F10x_StdPeriph_Driver/inc" -I"../../core-common-lib/STM32_USB-FS-Device_Driver/inc" -I"../../core-common-lib/CC3000_Host_Driver" -I"../../core-common-lib/SPARK_Firmware_Driver/inc" -I"../../core-communication-lib/lib/tropicssl/include" -I"../../core-communication-lib/src" -I"../inc" -Os -ffunction-sections -Wall -Werror -fno-exceptions -fno-rtti -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
