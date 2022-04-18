FOR INTERNAL USE ONLY !!!
Сontains parts of the source code from heaptrack project (LGPLv2 license) https://github.com/KDE/heaptrack

Tool for investigate ESP32 heap usage (malloc, free... )

How to use:

1. Copy mc.c to esp-idf/components/heap/

2. Edit esp-idf/components/heap/component.mk
a) Add mc.c to COMPONENT_OBJS in component.mk
b) Comment or remove "ifdef CONFIG_HEAP_TRACING" and "endif" for enable wrap

3. make menuconfig
a) Default log verbosity -> No output
a.1) Bootloader log verbosity -> No output
b) UART console baud rate -> 921600 (max for cp210x, esp32 support 5000000)
c) Disable watchdog and heap trace

4. Build and flash...

5. Write log from reset
a) minicom -D /dev/ttyUSB0 -C log.txt -b 921600 -> Ctrl+A,Q (exit)

6. Convert log
a) mc2ht log.txt log_ht.txt <full path to elf file; example /home/<user>/projects/ESP32/firmware/build/atom32.elf>
b) mc2ht_interpret log_ht.txt heaptrack.atom32.1 # used xtensa-esp32-elf-addr2line application for resolve address; xtensa-esp32-elf-addr2line must be accessible in PATH
c) gzip heaptrack.atom32.1

7) Use heaptrack_gui or heaptrack_print for investigate log heaptrack.atom32.1.gz (file format version 2)
