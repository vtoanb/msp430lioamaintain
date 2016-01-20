################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
Source/Common/printf.obj: C:/Users/CHE\ TAO\ MAY\ 03/Desktop/Zigbee_Module_Examples-20140822_version2233BETA/Common/printf.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-msp430_4.4.5/bin/cl430" -vmsp --abi=coffabi --code_model=small --data_model=small -O4 --opt_for_speed=0 --include_path="C:/ti/ccsv6/ccs_base/msp430/include" --include_path="C:/ti/ccsv6/tools/compiler/ti-cgt-msp430_4.4.5/include" --advice:power_severity=suppress --advice:power="all" -g --gcc --define=__MSP430G2553__ --define=LAUNCHPAD --define=ZM_PHY_SPI --define=xINCLUDE_IR_TEMPERATURE_SENSOR --define=xINCLUDE_COLOR_SENSOR --define=xINCLUDE_SUPPLY_VOLTAGE --define=xVERBOSE_ERROR_HANDLING --define=MODULE_INTERFACE_VERBOSE --define=xTESTING --diag_warning=225 --display_error_number --diag_wrap=off --printf_support=minimal --preproc_with_compile --preproc_dependency="Source/Common/printf.pp" --obj_directory="Source/Common" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

Source/Common/utilities.obj: C:/Users/CHE\ TAO\ MAY\ 03/Desktop/Zigbee_Module_Examples-20140822_version2233BETA/Common/utilities.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-msp430_4.4.5/bin/cl430" -vmsp --abi=coffabi --code_model=small --data_model=small -O4 --opt_for_speed=0 --include_path="C:/ti/ccsv6/ccs_base/msp430/include" --include_path="C:/ti/ccsv6/tools/compiler/ti-cgt-msp430_4.4.5/include" --advice:power_severity=suppress --advice:power="all" -g --gcc --define=__MSP430G2553__ --define=LAUNCHPAD --define=ZM_PHY_SPI --define=xINCLUDE_IR_TEMPERATURE_SENSOR --define=xINCLUDE_COLOR_SENSOR --define=xINCLUDE_SUPPLY_VOLTAGE --define=xVERBOSE_ERROR_HANDLING --define=MODULE_INTERFACE_VERBOSE --define=xTESTING --diag_warning=225 --display_error_number --diag_wrap=off --printf_support=minimal --preproc_with_compile --preproc_dependency="Source/Common/utilities.pp" --obj_directory="Source/Common" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


