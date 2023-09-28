#!/bin/bash

OS=`uname`
POSTFIX=""

if [[ $OS == MINGW* ]]; then
    POSTFIX=".exe"
else
    POSTFIX=""
fi

ba-elf-objdump -d     $1.elf    > $1.dmp
ba-elf-nm             $1.elf    > $1.map
ba-elf-readelf        $1.elf -a > 0_section_information.txt
ba-elf-nm -r          $1.elf    > 1_reverse_sort_symbol.txt
ba-elf-nm --size-sort $1.elf    > 2_symbol_by_size.txt
ba-elf-nm -p          $1.elf    > 3_no_sort_symbol.txt
ba-elf-nm -n          $1.elf    > 4_numeric_sort_symbol.txt
ba-elf-nm -l          $1.elf    > 5_symbol_and_line_number.txt
ba-elf-nm -g          $1.elf    > 6_external_symbol.txt
ba-elf-nm -a          $1.elf    > 7_debug_symbol.txt
ba-elf-nm -u          $1.elf    > 8_undefined_symbol.txt
ba-elf-nm -S          $1.elf    > 9_print_size_defined_symbol.txt

#Convert elf file to bin file ,but have no crc;
ba-elf-objcopy -O binary $1.elf  $1.bin

#added 2 crc bytes,format 32B(info) + 2B(CRC) = 34B
echo ./encrypt $1.bin 00000000


./ptn101_mcu_dsp_bin_merge${POSTFIX} 788528 ./$1.bin ./dsp_code_data.img ./$1_WITH_DSP.bin
