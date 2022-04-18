#!/bin/bash
# flashing ESP32
# Use env 'OPENOCDPATH'
# flag 'v' for enable verification

# save current dir
cwd=$(pwd)
# cd to script dir
DIR="$(cd "$(dirname "$0")" && pwd)"
cd $DIR

BIN="../build/atom32.bin"

print_usage () {
    echo "usage: prog.sh <metod: openocd_olimex, openocd_jlink> [v]"
    exit 1
}

if [[ -z "${ESP32_DEBUGGER}" ]]; then
    if [ "$#" -ge 1 ]; then
        ESP32_DEBUGGER=$1
    else
        print_usage
    fi
fi

if [ "$2" == "v" ]; then
    OPENOCDCMD="program_esp32 ${BIN} 0x10000 verify exit"
    
else
    OPENOCDCMD="program_esp32 ${BIN} 0x10000 exit"
    
fi

if [ "$ESP32_DEBUGGER" == "openocd_olimex" ]; then
    echo "use" $ESP32_DEBUGGER
    ${OPENOCDPATH}/bin/openocd -s ${OPENOCDPATH}/share/openocd/scripts -f openocd_olimex.cfg -c "${OPENOCDCMD}"

elif [ "$ESP32_DEBUGGER" == "openocd_jlink" ]; then
    echo "use" $ESP32_DEBUGGER
	${OPENOCDPATH}/bin/openocd -s ${OPENOCDPATH}/share/openocd/scripts -f openocd_jlink.cfg -c "${OPENOCDCMD}"

else
    print_usage
fi

cd $cwd
