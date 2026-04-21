################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/sh2-demo-nucleo/app/bno08x_i2c_hal.c \
../Drivers/sh2-demo-nucleo/app/bno08x_uart_hal.c \
../Drivers/sh2-demo-nucleo/app/button.c \
../Drivers/sh2-demo-nucleo/app/dbg.c \
../Drivers/sh2-demo-nucleo/app/i2c_hal.c \
../Drivers/sh2-demo-nucleo/app/usart.c 

OBJS += \
./Drivers/sh2-demo-nucleo/app/bno08x_i2c_hal.o \
./Drivers/sh2-demo-nucleo/app/bno08x_uart_hal.o \
./Drivers/sh2-demo-nucleo/app/button.o \
./Drivers/sh2-demo-nucleo/app/dbg.o \
./Drivers/sh2-demo-nucleo/app/i2c_hal.o \
./Drivers/sh2-demo-nucleo/app/usart.o 

C_DEPS += \
./Drivers/sh2-demo-nucleo/app/bno08x_i2c_hal.d \
./Drivers/sh2-demo-nucleo/app/bno08x_uart_hal.d \
./Drivers/sh2-demo-nucleo/app/button.d \
./Drivers/sh2-demo-nucleo/app/dbg.d \
./Drivers/sh2-demo-nucleo/app/i2c_hal.d \
./Drivers/sh2-demo-nucleo/app/usart.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/sh2-demo-nucleo/app/%.o Drivers/sh2-demo-nucleo/app/%.su Drivers/sh2-demo-nucleo/app/%.cyclo: ../Drivers/sh2-demo-nucleo/app/%.c Drivers/sh2-demo-nucleo/app/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -Dtimegm=mktime -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Drivers/sh2-demo-nucleo/app -I../Drivers/sh2-demo-nucleo/sh2 -I../Core/Inc -I../USB_HOST/App -I../USB_HOST/Target -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/CDC/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-sh2-2d-demo-2d-nucleo-2f-app

clean-Drivers-2f-sh2-2d-demo-2d-nucleo-2f-app:
	-$(RM) ./Drivers/sh2-demo-nucleo/app/bno08x_i2c_hal.cyclo ./Drivers/sh2-demo-nucleo/app/bno08x_i2c_hal.d ./Drivers/sh2-demo-nucleo/app/bno08x_i2c_hal.o ./Drivers/sh2-demo-nucleo/app/bno08x_i2c_hal.su ./Drivers/sh2-demo-nucleo/app/bno08x_uart_hal.cyclo ./Drivers/sh2-demo-nucleo/app/bno08x_uart_hal.d ./Drivers/sh2-demo-nucleo/app/bno08x_uart_hal.o ./Drivers/sh2-demo-nucleo/app/bno08x_uart_hal.su ./Drivers/sh2-demo-nucleo/app/button.cyclo ./Drivers/sh2-demo-nucleo/app/button.d ./Drivers/sh2-demo-nucleo/app/button.o ./Drivers/sh2-demo-nucleo/app/button.su ./Drivers/sh2-demo-nucleo/app/dbg.cyclo ./Drivers/sh2-demo-nucleo/app/dbg.d ./Drivers/sh2-demo-nucleo/app/dbg.o ./Drivers/sh2-demo-nucleo/app/dbg.su ./Drivers/sh2-demo-nucleo/app/i2c_hal.cyclo ./Drivers/sh2-demo-nucleo/app/i2c_hal.d ./Drivers/sh2-demo-nucleo/app/i2c_hal.o ./Drivers/sh2-demo-nucleo/app/i2c_hal.su ./Drivers/sh2-demo-nucleo/app/usart.cyclo ./Drivers/sh2-demo-nucleo/app/usart.d ./Drivers/sh2-demo-nucleo/app/usart.o ./Drivers/sh2-demo-nucleo/app/usart.su

.PHONY: clean-Drivers-2f-sh2-2d-demo-2d-nucleo-2f-app

