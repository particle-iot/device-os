
# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../lib/tropicssl/library/aes.c \
../lib/tropicssl/library/bignum.c \
../lib/tropicssl/library/padlock.c \
../lib/tropicssl/library/rsa.c \
../lib/tropicssl/library/sha1.c 

OBJS += \
./lib/tropicssl/library/aes.o \
./lib/tropicssl/library/bignum.o \
./lib/tropicssl/library/padlock.o \
./lib/tropicssl/library/rsa.o \
./lib/tropicssl/library/sha1.o 

C_DEPS += \
./lib/tropicssl/library/aes.d \
./lib/tropicssl/library/bignum.d \
./lib/tropicssl/library/padlock.d \
./lib/tropicssl/library/rsa.d \
./lib/tropicssl/library/sha1.d 


# Each subdirectory must supply rules for building sources it contributes
lib/tropicssl/library/%.o: ../lib/tropicssl/library/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Sourcery Windows GCC C Compiler'
	arm-none-eabi-gcc -I../lib/tropicssl/include -I../src -Os -ffunction-sections -Wall -c -fmessage-length=0 -MMD -MP -MF -MT -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o $@ $<
	@echo 'Finished building: $<'
	@echo ' '


