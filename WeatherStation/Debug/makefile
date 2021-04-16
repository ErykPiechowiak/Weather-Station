################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

CG_TOOL_ROOT := D:/Code Composer Studio 6/CSS/ccsv6/tools/compiler/ti-cgt-arm_5.2.2

ORDERED_OBJS += \
"./DS3231.obj" \
"./background.obj" \
"./background3.obj" \
"./bme280.obj" \
"./fpu.obj" \
"./ili9341_240x320x262K.obj" \
"./lm3s9b95_startup_ccs.obj" \
"./main.obj" \
"./mmc.obj" \
"./touch.obj" \
"D:/CCS_libraries/Stellaris Peripheral/driverlib/rvmdk-cm3/driverlib-cm3.lib" \
"D:/CCS_libraries/Stellaris Peripheral/grlib/ccs-cm3/Debug/grlib-cm3.lib" \
"../lm3s9b95.cmd" \
$(GEN_CMDS__FLAG) \
-l"libc.a" \

-include ../makefile.init

RM := DEL /F
RMDIR := RMDIR /S/Q

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include subdir_rules.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(S62_DEPS)),)
-include $(S62_DEPS)
endif
ifneq ($(strip $(C64_DEPS)),)
-include $(C64_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(S55_DEPS)),)
-include $(S55_DEPS)
endif
ifneq ($(strip $(C67_DEPS)),)
-include $(C67_DEPS)
endif
ifneq ($(strip $(CLA_DEPS)),)
-include $(CLA_DEPS)
endif
ifneq ($(strip $(C??_DEPS)),)
-include $(C??_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(S??_DEPS)),)
-include $(S??_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(C62_DEPS)),)
-include $(C62_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(K_DEPS)),)
-include $(K_DEPS)
endif
ifneq ($(strip $(C43_DEPS)),)
-include $(C43_DEPS)
endif
ifneq ($(strip $(INO_DEPS)),)
-include $(INO_DEPS)
endif
ifneq ($(strip $(S67_DEPS)),)
-include $(S67_DEPS)
endif
ifneq ($(strip $(SA_DEPS)),)
-include $(SA_DEPS)
endif
ifneq ($(strip $(S43_DEPS)),)
-include $(S43_DEPS)
endif
ifneq ($(strip $(OPT_DEPS)),)
-include $(OPT_DEPS)
endif
ifneq ($(strip $(PDE_DEPS)),)
-include $(PDE_DEPS)
endif
ifneq ($(strip $(S64_DEPS)),)
-include $(S64_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(C55_DEPS)),)
-include $(C55_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXE_OUTPUTS += \
WeatherStation.out \

EXE_OUTPUTS__QUOTED += \
"WeatherStation.out" \

BIN_OUTPUTS += \
WeatherStation.hex \

BIN_OUTPUTS__QUOTED += \
"WeatherStation.hex" \


# All Target
all: WeatherStation.out

# Tool invocations
WeatherStation.out: $(OBJS) $(CMD_SRCS) $(LIB_SRCS) $(GEN_CMDS)
	@echo 'Building target: $@'
	@echo 'Invoking: ARM Linker'
	"D:/Code Composer Studio 6/CSS/ccsv6/tools/compiler/ti-cgt-arm_5.2.2/bin/armcl" -mv7M3 --code_state=16 --abi=eabi -me -g --gcc --define="ccs" --define=ccs="ccs" --define=TARGET_IS_TEMPEST_RB1 --define=PART_LM3S9B95 --display_error_number --diag_warning=225 --diag_wrap=off -z -m"WeatherStation.map" --heap_size=0 --stack_size=2048 -i"D:/Code Composer Studio 6/CSS/ccsv6/tools/compiler/ti-cgt-arm_5.2.2/lib" -i"D:/Code Composer Studio 6/CSS/ccsv6/tools/compiler/ti-cgt-arm_5.2.2/include" --reread_libs --warn_sections --display_error_number --diag_wrap=off --xml_link_info="WeatherStation_linkInfo.xml" --rom_model -o "WeatherStation.out" $(ORDERED_OBJS)
	@echo 'Finished building target: $@'
	@echo ' '

WeatherStation.hex: $(EXE_OUTPUTS)
	@echo 'Invoking: ARM Hex Utility'
	"D:/Code Composer Studio 6/CSS/ccsv6/tools/compiler/ti-cgt-arm_5.2.2/bin/armhex"  -o "WeatherStation.hex" $(EXE_OUTPUTS__QUOTED)
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXE_OUTPUTS__QUOTED)$(BIN_OUTPUTS__QUOTED)
	-$(RM) "DS3231.pp" "background.pp" "background3.pp" "bme280.pp" "fpu.pp" "ili9341_240x320x262K.pp" "lm3s9b95_startup_ccs.pp" "main.pp" "mmc.pp" "touch.pp" 
	-$(RM) "DS3231.obj" "background.obj" "background3.obj" "bme280.obj" "fpu.obj" "ili9341_240x320x262K.obj" "lm3s9b95_startup_ccs.obj" "main.obj" "mmc.obj" "touch.obj" 
	-@echo 'Finished clean'
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets

