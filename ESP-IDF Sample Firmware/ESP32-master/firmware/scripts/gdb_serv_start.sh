#!/bin/bash

# save current dir
cwd=$(pwd)
# cd to script dir
DIR="$(cd "$(dirname "$0")" && pwd)"
cd $DIR

print_usage () {
    echo "usage: gdb_serv_start.sh <metod: openocd_olimex, openocd_jlink>"
    exit 1
}

if [[ -z "${ESP32_DEBUGGER}" ]]; then
    if [ "$#" -ge 1 ]; then
        ESP32_DEBUGGER=$1
    else
        print_usage
    fi
fi

if [ "$ESP32_DEBUGGER" == "openocd_olimex" ]; then
    echo "start" $ESP32_DEBUGGER
    ${OPENOCDPATH}/bin/openocd -s ${OPENOCDPATH}/share/openocd/scripts -f openocd_olimex.cfg
elif [ "$ESP32_DEBUGGER" == "openocd_jlink" ]; then
    echo "start" $ESP32_DEBUGGER
    ${OPENOCDPATH}/bin/openocd -s ${OPENOCDPATH}/share/openocd/scripts -f openocd_jlink.cfg
else
    print_usage
fi

cd $cwd