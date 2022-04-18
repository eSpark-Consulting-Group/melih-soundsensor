#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

COMPONENT_ADD_INCLUDEDIRS := include

COMPONENT_SRCDIRS := src src/commands src/devices src/items src/handlers

COMPONENT_EMBED_FILES := resources/zwave_devices_info.dat resources/then_rule_blocks.dat resources/when_rule_blocks.dat
