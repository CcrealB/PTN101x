#!/bin/bash

if [[ $1 == clean ]]; then
    ninja clean
    rm -rf *.txt *.bin *.map *.cmake *.elf *.dmp CMakeFiles Makefile *.ninja .ninja_deps .ninja_log
    exit
fi

start=$(date +%s)

echo -e "\n==============BUILD START=================\n"

echo -e "-------------- Pre Build -----------------\n"
rm *.txt *.bin *.map *.dmp *.elf
cmake ../../.. -G "Ninja"

echo -e "\n-------------- Main Build ----------------\n"
cmake --build .

echo -e "\n-------------- Post Build ----------------\n"
echo "Merging DSP image..."
ELF_FILE=`find *.elf`
TARGET_NAME=${ELF_FILE%.*}
chmod +x ./encrypt.sh ./ptn101_mcu_dsp_bin_merge && ./encrypt.sh ${TARGET_NAME}

echo -e "\nShowing the size of executable"
ba-elf-size -B ${TARGET_NAME}.elf
echo -e "Invoking: BA ELF GNU objcopy"
ba-elf-objcopy -O binary ${TARGET_NAME}.elf  ${TARGET_NAME}.bin

echo -e "\n==============BUILD FINISHED==============\n"
end=$(date +%s)
take=$(( end - start ))
echo It took ${take}s to build.

