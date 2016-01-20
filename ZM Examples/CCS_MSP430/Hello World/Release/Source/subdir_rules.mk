################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
Source/example_hello_world.obj: C:/Users/CHE\ TAO\ MAY\ 03/Desktop/Zigbee_Module_Examples-20140822_version2233BETA/ZM\ Examples/example_hello_world.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-msp430_4.4.5/bin/cl430" -vmsp --abi=eabi --code_model=small --data_model=small -O2 --include_path="C:/ti/ccsv6/ccs_base/msp430/include" --include_path="C:/ti/ccsv6/tools/compiler/ti-cgt-msp430_4.4.5/include" --advice:power_severity=suppress --advice:power="all" -g --relaxed_ansi --gcc --define=__MSP430G2553__ --define=LAUNCHPAD --diag_warning=225 --display_error_number --diag_wrap=off --printf_support=minimal --preproc_with_compile --preproc_dependency="Source/example_hello_world.pp" --obj_directory="Source" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


