################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/minmea/minmea.c 

OBJS += \
./Drivers/minmea/minmea.o 

C_DEPS += \
./Drivers/minmea/minmea.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/minmea/minmea.o: ../Drivers/minmea/minmea.c Drivers/minmea/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -Dtimegm=mktime -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Drivers/sh2-demo-nucleo/app -I../Drivers/minmea -I../Drivers/sh2-demo-nucleo/sh2 -I../Core/Inc -I../USB_HOST/App -I../USB_HOST/Target -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/CDC/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-minmea

clean-Drivers-2f-minmea:
	-$(RM) ./Drivers/minmea/minmea.cyclo ./Drivers/minmea/minmea.d ./Drivers/minmea/minmea.o ./Drivers/minmea/minmea.su

.PHONY: clean-Drivers-2f-minmea

