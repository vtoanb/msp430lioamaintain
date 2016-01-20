################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
Source/Messages/header.obj: C:/Users/CHE\ TAO\ MAY\ 03/Desktop/Zigbee_Module_Examples-20140822_version2233BETA/ZM\ Examples/Messages/header.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-msp430_4.4.5/bin/cl430" -vmsp --abi=coffabi --data_model=small --code_model=small -O4 --opt_for_speed=0 --use_hw_mpy=none --include_path="C:/ti/ccsv6/ccs_base/msp430/include" --include_path="C:/ti/ccsv6/tools/compiler/ti-cgt-msp430_4.4.5/include" --advice:power_severity=suppress --advice:power="all" -g --gcc --define=__MSP430G2553__ --define=LAUNCHPAD --define=ZM_PHY_SPI --define=REGION_NORTH_AMERICA --define=xREGION_EUROPE --define=INCLUDE_RGB_LED --define=xUSE_STDIO --define=VERBOSE_ERROR_HANDLING --define=xTESTING --diag_warning=225 --diag_wrap=off --display_error_number --printf_support=minimal --preproc_with_compile --preproc_dependency="Source/Messages/header.pp" --obj_directory="Source/Messages" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

Source/Messages/infoMessage.obj: C:/Users/CHE\ TAO\ MAY\ 03/Desktop/Zigbee_Module_Examples-20140822_version2233BETA/ZM\ Examples/Messages/infoMessage.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-msp430_4.4.5/bin/cl430" -vmsp --abi=coffabi --data_model=small --code_model=small -O4 --opt_for_speed=0 --use_hw_mpy=none --include_path="C:/ti/ccsv6/ccs_base/msp430/include" --include_path="C:/ti/ccsv6/tools/compiler/ti-cgt-msp430_4.4.5/include" --advice:power_severity=suppress --advice:power="all" -g --gcc --define=__MSP430G2553__ --define=LAUNCHPAD --define=ZM_PHY_SPI --define=REGION_NORTH_AMERICA --define=xREGION_EUROPE --define=INCLUDE_RGB_LED --define=xUSE_STDIO --define=VERBOSE_ERROR_HANDLING --define=xTESTING --diag_warning=225 --diag_wrap=off --display_error_number --printf_support=minimal --preproc_with_compile --preproc_dependency="Source/Messages/infoMessage.pp" --obj_directory="Source/Messages" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

Source/Messages/oids.obj: C:/Users/CHE\ TAO\ MAY\ 03/Desktop/Zigbee_Module_Examples-20140822_version2233BETA/ZM\ Examples/Messages/oids.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-msp430_4.4.5/bin/cl430" -vmsp --abi=coffabi --data_model=small --code_model=small -O4 --opt_for_speed=0 --use_hw_mpy=none --include_path="C:/ti/ccsv6/ccs_base/msp430/include" --include_path="C:/ti/ccsv6/tools/compiler/ti-cgt-msp430_4.4.5/include" --advice:power_severity=suppress --advice:power="all" -g --gcc --define=__MSP430G2553__ --define=LAUNCHPAD --define=ZM_PHY_SPI --define=REGION_NORTH_AMERICA --define=xREGION_EUROPE --define=INCLUDE_RGB_LED --define=xUSE_STDIO --define=VERBOSE_ERROR_HANDLING --define=xTESTING --diag_warning=225 --diag_wrap=off --display_error_number --printf_support=minimal --preproc_with_compile --preproc_dependency="Source/Messages/oids.pp" --obj_directory="Source/Messages" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


