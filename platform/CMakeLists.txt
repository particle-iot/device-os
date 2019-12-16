cmake_minimum_required(VERSION 3.16)

project(libparticle_platform)

add_library(
    particle_platform
    MCU/shared/STM32/src/hw_system_flags.c
    MCU/shared/STM32/src/hw_ticks.c
    MCU/STM32F1xx/SPARK_Firmware_Driver/src/cc3000_spi.c
    MCU/STM32F1xx/SPARK_Firmware_Driver/src/hw_config.c
    MCU/STM32F1xx/SPARK_Firmware_Driver/src/spi_bus.c
    MCU/STM32F1xx/SPARK_Firmware_Driver/src/sst25vf_spi.c
    MCU/STM32F1xx/SPARK_Firmware_Driver/src/system_stm32f10x.c
    MCU/STM32F1xx/SPARK_Firmware_Driver/src/usb_endp.c
    MCU/STM32F1xx/SPARK_Firmware_Driver/src/usb_pwr.c
    MCU/STM32F1xx/STM32_StdPeriph_Driver/src/misc.c
    MCU/STM32F1xx/STM32_StdPeriph_Driver/src/stm32f10x_adc.c
    MCU/STM32F1xx/STM32_StdPeriph_Driver/src/stm32f10x_bkp.c
    MCU/STM32F1xx/STM32_StdPeriph_Driver/src/stm32f10x_can.c
    MCU/STM32F1xx/STM32_StdPeriph_Driver/src/stm32f10x_cec.c
    MCU/STM32F1xx/STM32_StdPeriph_Driver/src/stm32f10x_crc.c
    MCU/STM32F1xx/STM32_StdPeriph_Driver/src/stm32f10x_dac.c
    MCU/STM32F1xx/STM32_StdPeriph_Driver/src/stm32f10x_dbgmcu.c
    MCU/STM32F1xx/STM32_StdPeriph_Driver/src/stm32f10x_dma.c
    MCU/STM32F1xx/STM32_StdPeriph_Driver/src/stm32f10x_exti.c
    MCU/STM32F1xx/STM32_StdPeriph_Driver/src/stm32f10x_flash.c
    MCU/STM32F1xx/STM32_StdPeriph_Driver/src/stm32f10x_fsmc.c
    MCU/STM32F1xx/STM32_StdPeriph_Driver/src/stm32f10x_gpio.c
    MCU/STM32F1xx/STM32_StdPeriph_Driver/src/stm32f10x_i2c.c
    MCU/STM32F1xx/STM32_StdPeriph_Driver/src/stm32f10x_iwdg.c
    MCU/STM32F1xx/STM32_StdPeriph_Driver/src/stm32f10x_pwr.c
    MCU/STM32F1xx/STM32_StdPeriph_Driver/src/stm32f10x_rcc.c
    MCU/STM32F1xx/STM32_StdPeriph_Driver/src/stm32f10x_rtc.c
    MCU/STM32F1xx/STM32_StdPeriph_Driver/src/stm32f10x_sdio.c
    MCU/STM32F1xx/STM32_StdPeriph_Driver/src/stm32f10x_spi.c
    MCU/STM32F1xx/STM32_StdPeriph_Driver/src/stm32f10x_tim.c
    MCU/STM32F1xx/STM32_StdPeriph_Driver/src/stm32f10x_usart.c
    MCU/STM32F1xx/STM32_StdPeriph_Driver/src/stm32f10x_wwdg.c
    MCU/STM32F1xx/STM32_USB_Device_Driver/src/usb_core.c
    MCU/STM32F1xx/STM32_USB_Device_Driver/src/usb_init.c
    MCU/STM32F1xx/STM32_USB_Device_Driver/src/usb_int.c
    MCU/STM32F1xx/STM32_USB_Device_Driver/src/usb_mem.c
    MCU/STM32F1xx/STM32_USB_Device_Driver/src/usb_regs.c
    MCU/STM32F1xx/STM32_USB_Device_Driver/src/usb_sil.c
    NET/CC3000/CC3000_Host_Driver/cc3000_common.c
    NET/CC3000/CC3000_Host_Driver/evnt_handler.c
    NET/CC3000/CC3000_Host_Driver/hci.c
    NET/CC3000/CC3000_Host_Driver/netapp.c
    NET/CC3000/CC3000_Host_Driver/nvmem.c
    NET/CC3000/CC3000_Host_Driver/security.c
    NET/CC3000/CC3000_Host_Driver/socket.c
    NET/CC3000/CC3000_Host_Driver/wlan.c
)

target_compile_definitions(
    particle_platform
# Solves the duplicate definition of `timeval`, included by gcc-arm
# Required on Spark Core, due to the CC3000_Host_Driver
    INTERFACE _TIMEVAL_DEFINED=1
)

target_include_directories(
    particle_platform
    PUBLIC MCU/shared/STM32/inc
    PUBLIC MCU/STM32F1xx/CMSIS/Device/ST/Include
    PUBLIC MCU/STM32F1xx/CMSIS/Include
    PUBLIC MCU/STM32F1xx/SPARK_Firmware_Driver/inc
    PUBLIC MCU/STM32F1xx/STM32_StdPeriph_Driver/inc
    PUBLIC MCU/STM32F1xx/STM32_USB_Device_Driver/inc
    PUBLIC NET/CC3000/CC3000_Host_Driver
#[=[
Items below this comment have been added to resolve a cyclic dependency.
Any items added below should have a related Clubhouse story identifying the
nature of the dependency and the steps required to resolve it.
#]=]
    PRIVATE ../hal/inc
    PRIVATE ../hal/shared
    PRIVATE ../hal/src/core
    PRIVATE ../services/inc
)
