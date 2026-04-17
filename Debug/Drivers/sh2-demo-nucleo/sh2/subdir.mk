################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/sh2-demo-nucleo/sh2/euler.c \
../Drivers/sh2-demo-nucleo/sh2/sh2.c \
../Drivers/sh2-demo-nucleo/sh2/sh2_SensorValue.c \
../Drivers/sh2-demo-nucleo/sh2/sh2_util.c \
../Drivers/sh2-demo-nucleo/sh2/shtp.c 

OBJS += \
./Drivers/sh2-demo-nucleo/sh2/euler.o \
./Drivers/sh2-demo-nucleo/sh2/sh2.o \
./Drivers/sh2-demo-nucleo/sh2/sh2_SensorValue.o \
./Drivers/sh2-demo-nucleo/sh2/sh2_util.o \
./Drivers/sh2-demo-nucleo/sh2/shtp.o 

C_DEPS += \
./Drivers/sh2-demo-nucleo/sh2/euler.d \
./Drivers/sh2-demo-nucleo/sh2/sh2.d \
./Drivers/sh2-demo-nucleo/sh2/sh2_SensorValue.d \
./Drivers/sh2-demo-nucleo/sh2/sh2_util.d \
./Drivers/sh2-demo-nucleo/sh2/shtp.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/sh2-demo-nucleo/sh2/%.o Drivers/sh2-demo-nucleo/sh2/%.su Drivers/sh2-demo-nucleo/sh2/%.cyclo: ../Drivers/sh2-demo-nucleo/sh2/%.c Drivers/sh2-demo-nucleo/sh2/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -Dtimegm=mktime -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Drivers/sh2-demo-nucleo/app -I../Drivers/sh2-demo-nucleo/sh2 -I../Core/Inc -I../USB_HOST/App -I../USB_HOST/Target -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/CDC/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-sh2-2d-demo-2d-nucleo-2f-sh2

clean-Drivers-2f-sh2-2d-demo-2d-nucleo-2f-sh2:
	-$(RM) ./Drivers/sh2-demo-nucleo/sh2/euler.cyclo ./Drivers/sh2-demo-nucleo/sh2/euler.d ./Drivers/sh2-demo-nucleo/sh2/euler.o ./Drivers/sh2-demo-nucleo/sh2/euler.su ./Drivers/sh2-demo-nucleo/sh2/sh2.cyclo ./Drivers/sh2-demo-nucleo/sh2/sh2.d ./Drivers/sh2-demo-nucleo/sh2/sh2.o ./Drivers/sh2-demo-nucleo/sh2/sh2.su ./Drivers/sh2-demo-nucleo/sh2/sh2_SensorValue.cyclo ./Drivers/sh2-demo-nucleo/sh2/sh2_SensorValue.d ./Drivers/sh2-demo-nucleo/sh2/sh2_SensorValue.o ./Drivers/sh2-demo-nucleo/sh2/sh2_SensorValue.su ./Drivers/sh2-demo-nucleo/sh2/sh2_util.cyclo ./Drivers/sh2-demo-nucleo/sh2/sh2_util.d ./Drivers/sh2-demo-nucleo/sh2/sh2_util.o ./Drivers/sh2-demo-nucleo/sh2/sh2_util.su ./Drivers/sh2-demo-nucleo/sh2/shtp.cyclo ./Drivers/sh2-demo-nucleo/sh2/shtp.d ./Drivers/sh2-demo-nucleo/sh2/shtp.o ./Drivers/sh2-demo-nucleo/sh2/shtp.su

.PHONY: clean-Drivers-2f-sh2-2d-demo-2d-nucleo-2f-sh2

