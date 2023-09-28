# Set the main target of this build. ${THE_TARGET} will determine the name of the output elf file.
set(THE_TARGET PTN101x_MCU)

# Set project file locations
list(APPEND PTN101_SRC_DIR "${PTN101_TOP_DIR}/host")
list(APPEND PTN101_SRC_DIR "${PTN101_TOP_DIR}/controller")
list(APPEND PTN101_SRC_DIR "${PTN101_TOP_DIR}/rw_ble")
list(APPEND PTN101_SRC_DIR "${PTN101_TOP_DIR}/projects/app_full_fun/app")
list(APPEND PTN101_SRC_DIR "${PTN101_TOP_DIR}/projects/app_full_fun/app/SYS_APP")

# Exclude source folders from build.
set(EXCLUDE_DIR 
    "^${PTN101_TOP_DIR}/controller/core/"
    "^${PTN101_TOP_DIR}/host/bluetooth/"
    "^${PTN101_TOP_DIR}/rw_ble/"
    "^${PTN101_TOP_DIR}/rw_ble/Src/ip/"
)

# Set library path
set(LIBS "${PTN101_TOP_DIR}/host/libs/libPTN101_CONTROLLER_LIB.a"
         "${PTN101_TOP_DIR}/host/libs/libPTN101_HOST_LIB.a"
         "${PTN101_TOP_DIR}/host/libs/libPTN101_RWBLE_LIB.a"
         "${PTN101_TOP_DIR}/host/libs/Mp3Lib/libmp3encoder.a"
         "${PTN101_TOP_DIR}/host/libs/AAC/libaac_decoder.a"
        #  "${PTN101_TOP_DIR}/host/libs/AEC/libaec.a"
         "${PTN101_TOP_DIR}/host/libs/RESAMPLE/libsrc_244.a"
         "${PTN101_TOP_DIR}/host/libs/RESAMPLE/libsrc_248.a"
        #  "${PTN101_TOP_DIR}/host/libs/RESAMPLE/libsrc.a"
         "${PTN101_TOP_DIR}/host/libs/libPTN101x_MCU_LIB.a"
)

# Set LD file path
set(LD_PATH "${PTN101_TOP_DIR}/projects/app_full_fun")

# Set project specific include paths
set(INC_PATH
"${PTN101_TOP_DIR}/projects/app_full_fun/app"
"${PTN101_TOP_DIR}/projects/app_full_fun/app/SYS_APP"
${INC_PATH}	
)
