################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/sh2-demo-nucleo/main/stm32f4xx_hal_timebase_TIM.c \
../Drivers/sh2-demo-nucleo/main/system_stm32f4xx.c 

OBJS += \
./Drivers/sh2-demo-nucleo/main/stm32f4xx_hal_timebase_TIM.o \
./Drivers/sh2-demo-nucleo/main/system_stm32f4xx.o 

C_DEPS += \
./Drivers/sh2-demo-nucleo/main/stm32f4xx_hal_timebase_TIM.d \
./Drivers/sh2-demo-nucleo/main/system_stm32f4xx.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/sh2-demo-nucleo/main/%.o Drivers/sh2-demo-nucleo/main/%.su Drivers/sh2-demo-nucleo/main/%.cyclo: ../Drivers/sh2-demo-nucleo/main/%.c Drivers/sh2-demo-nucleo/main/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Drivers/sh2-demo-nucleo/app -I../Drivers/sh2-demo-nucleo/sh2 -I../Core/Inc -I../USB_HOST/App -I../USB_HOST/Target -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/CDC/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-sh2-2d-demo-2d-nucleo-2f-main

clean-Drivers-2f-sh2-2d-demo-2d-nucleo-2f-main:
	-$(RM) ./Drivers/sh2-demo-nucleo/main/stm32f4xx_hal_timebase_TIM.cyclo ./Drivers/sh2-demo-nucleo/main/stm32f4xx_hal_timebase_TIM.d ./Drivers/sh2-demo-nucleo/main/stm32f4xx_hal_timebase_TIM.o ./Drivers/sh2-demo-nucleo/main/stm32f4xx_hal_timebase_TIM.su ./Drivers/sh2-demo-nucleo/main/system_stm32f4xx.cyclo ./Drivers/sh2-demo-nucleo/main/system_stm32f4xx.d ./Drivers/sh2-demo-nucleo/main/system_stm32f4xx.o ./Drivers/sh2-demo-nucleo/main/system_stm32f4xx.su

.PHONY: clean-Drivers-2f-sh2-2d-demo-2d-nucleo-2f-main

