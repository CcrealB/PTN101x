WINOS="Windows"

if [[ $OS == *$WINOS* ]]; then
    BINPOSTFIX=".exe"
else
    BINPOSTFIX=""
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
echo ./encrypt$BINPOSTFIX $1.bin 00000000

# calculate dsp merge address according mcu_bin size, 4KB aligned and reserve 4KB for 32Byte dsp info space
bin_size=$(stat --format=%s $1.bin)
bin_size_crc=`expr $bin_size \* 34 / 32 + 32`
merge_addr=`expr $bin_size_crc / 4096 \* 4 + 8`
if [ $merge_addr -lt 512 ]
then
   merge_addr=512
fi
echo "bin_size:$bin_size, bin_size_crc:$bin_size_crc, merge_addr:$merge_addr K"

./ptn101_mcu_dsp_bin_merge$BINPOSTFIX $merge_addr ./$1.bin ./dsp_code_data.img ./$1_WITH_DSP.bin
# ./ptn101_mcu_dsp_bin_merge$BINPOSTFIX $merge_addr ./$1.bin ./dsp_code_data_NOALG.img ./$1_WITH_DSP_test.bin


