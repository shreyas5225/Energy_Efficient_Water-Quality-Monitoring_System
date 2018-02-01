################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include emlib/subdir.mk
-include Drivers/subdir.mk
-include CMSIS/EFM32LG/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: STK3600_leuart.axf

# Tool invocations
STK3600_leuart.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GNU ARM C Linker'
	arm-none-eabi-gcc -g -gdwarf-2 -mcpu=cortex-m3 -mthumb -T "STK3600_leuart.ld" -Xlinker --gc-sections -Xlinker -Map="STK3600_leuart.map" --specs=nano.specs -o STK3600_leuart.axf "./CMSIS/EFM32LG/startup_efm32lg.o" "./CMSIS/EFM32LG/system_efm32lg.o" "./Drivers/dmactrl.o" "./emlib/em_assert.o" "./emlib/em_cmu.o" "./emlib/em_dma.o" "./emlib/em_emu.o" "./emlib/em_gpio.o" "./emlib/em_letimer.o" "./emlib/em_leuart.o" "./emlib/em_system.o" "./emlib/em_usart.o" "./src/main.o" -Wl,--start-group -lgcc -lc -lnosys -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

	@echo 'Building hex file: STK3600_leuart.hex'
	arm-none-eabi-objcopy -O ihex "STK3600_leuart.axf" "STK3600_leuart.hex"
	@echo ' '

	@echo 'Building bin file: STK3600_leuart.bin'
	arm-none-eabi-objcopy -O binary "STK3600_leuart.axf" "STK3600_leuart.bin"
	@echo ' '

	@echo 'Building s37 file: STK3600_leuart.s37'
	arm-none-eabi-objcopy -O srec "STK3600_leuart.axf" "STK3600_leuart.s37"
	@echo ' '

	@echo 'Running size tool'
	arm-none-eabi-size "STK3600_leuart.axf"
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) STK3600_leuart.axf
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
