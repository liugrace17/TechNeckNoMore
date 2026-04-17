################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/sh2-demo-nucleo/rvc/rvc.c 

OBJS += \
./Drivers/sh2-demo-nucleo/rvc/rvc.o 

C_DEPS += \
./Drivers/sh2-demo-nucleo/rvc/rvc.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/sh2-demo-nucleo/rvc/%.o Drivers/sh2-demo-nucleo/rvc/%.su Drivers/sh2-demo-nucleo/rvc/%.cyclo: ../Drivers/sh2-demo-nucleo/rvc/%.c Drivers/sh2-demo-nucleo/rvc/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -Dtimegm=mktime -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Drivers/sh2-demo-nucleo/app -I../Drivers/sh2-demo-nucleo/sh2 -I../Core/Inc -I../USB_HOST/App -I../USB_HOST/Target -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/CDC/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-sh2-2d-demo-2d-nucleo-2f-rvc

clean-Drivers-2f-sh2-2d-demo-2d-nucleo-2f-rvc:
	-$(RM) ./Drivers/sh2-demo-nucleo/rvc/rvc.cyclo ./Drivers/sh2-demo-nucleo/rvc/rvc.d ./Drivers/sh2-demo-nucleo/rvc/rvc.o ./Drivers/sh2-demo-nucleo/rvc/rvc.su

.PHONY: clean-Drivers-2f-sh2-2d-demo-2d-nucleo-2f-rvc

