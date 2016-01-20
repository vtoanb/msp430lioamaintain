################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

CG_TOOL_ROOT := C:/ti/ccsv6/tools/compiler/ti-cgt-msp430_4.4.5

ORDERED_OBJS += \
"./Source/example_network_explorer.obj" \
"./Source/module_example_utils.obj" \
"./Source/ZM/af.obj" \
"./Source/ZM/application_configuration.obj" \
"./Source/ZM/module.obj" \
"./Source/ZM/module_errors.obj" \
"./Source/ZM/module_utilities.obj" \
"./Source/ZM/zdo.obj" \
"./Source/ZM/zm_phy_spi.obj" \
"./Source/HAL/hal_launchpad.obj" \
"./Source/Common/printf.obj" \
"./Source/Common/utilities.obj" \
"../lnk_msp430g2553.cmd" \
$(GEN_CMDS__FLAG) \
-l"libc.a" \

-include ../makefile.init

RM := DEL /F
RMDIR := RMDIR /S/Q

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include Source/subdir_vars.mk
-include Source/ZM/subdir_vars.mk
-include Source/HAL/subdir_vars.mk
-include Source/Common/subdir_vars.mk
-include subdir_rules.mk
-include Source/subdir_rules.mk
-include Source/ZM/subdir_rules.mk
-include Source/HAL/subdir_rules.mk
-include Source/Common/subdir_rules.mk
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
Network\ Explorer.out \

EXE_OUTPUTS__QUOTED += \
"Network Explorer.out" \


# All Target
all: Network\ Explorer.out

# Tool invocations
Network\ Explorer.out: $(OBJS) $(CMD_SRCS) $(GEN_CMDS)
	@echo 'Building target: $@'
	@echo 'Invoking: MSP430 Linker'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-msp430_4.4.5/bin/cl430" -vmsp --abi=eabi --code_model=small --data_model=small -O2 --advice:power_severity=suppress --advice:power="all" -g --relaxed_ansi --gcc --define=__MSP430G2553__ --define=LAUNCHPAD --define=ZM_PHY_SPI --define=LAUNCHPAD_V1 --diag_warning=225 --display_error_number --diag_wrap=off --printf_support=minimal -z -m"Network Explorer.map" --heap_size=80 --stack_size=80 -i"C:/ti/ccsv6/ccs_base/msp430/include" -i"C:/ti/ccsv6/tools/compiler/ti-cgt-msp430_4.4.5/lib" -i"C:/ti/ccsv6/tools/compiler/ti-cgt-msp430_4.4.5/include" --reread_libs --warn_sections --display_error_number --diag_wrap=off --xml_link_info="Network Explorer_linkInfo.xml" --rom_model -o "Network Explorer.out" $(ORDERED_OBJS)
	@echo 'Finished building target: $@'
	@echo ' '
	@$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXE_OUTPUTS__QUOTED)
	-$(RM) "Source\example_network_explorer.pp" "Source\module_example_utils.pp" "Source\ZM\af.pp" "Source\ZM\application_configuration.pp" "Source\ZM\module.pp" "Source\ZM\module_errors.pp" "Source\ZM\module_utilities.pp" "Source\ZM\zdo.pp" "Source\ZM\zm_phy_spi.pp" "Source\HAL\hal_launchpad.pp" "Source\Common\printf.pp" "Source\Common\utilities.pp" 
	-$(RM) "Source\example_network_explorer.obj" "Source\module_example_utils.obj" "Source\ZM\af.obj" "Source\ZM\application_configuration.obj" "Source\ZM\module.obj" "Source\ZM\module_errors.obj" "Source\ZM\module_utilities.obj" "Source\ZM\zdo.obj" "Source\ZM\zm_phy_spi.obj" "Source\HAL\hal_launchpad.obj" "Source\Common\printf.obj" "Source\Common\utilities.obj" 
	-@echo 'Finished clean'
	-@echo ' '

post-build:
	-@echo 'Create flash image: Intel-HEX'
	-"C:/ti/ccsv6/tools/compiler/ti-cgt-msp430_4.4.5/bin/hex430" -i "Network Explorer.out" -o "NetworkExplorer.hex" -order MS -romwidth 16
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets

