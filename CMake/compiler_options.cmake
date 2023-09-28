# SDK common compiler and linker options

function(apply_compile_options_to_target THETARGET)
  get_target_property(target_type ${THETARGET} TYPE)
  if(target_type AND target_type MATCHES "EXECUTABLE")
    set(DEBUG_LEVEL -g3)
  endif ()
  target_compile_options(${THETARGET}
    PRIVATE
    -mcpu=ba22 
    -mle 
    # -mno-hard-float 
    -mhard-float 
    -mfpu=v2sp 
    -mhard-mul 
    -mhard-div 
    -mdsp 
    -mmac 
    -mtj-rodata 
    -mabi=3 
    -G1024 
    -membedded 
    -malign32 
    -specs=raptor.specs 
    -DBA22_DEee 
    -Os 
    -flto 
    -G 100000   
    ${DEBUG_LEVEL} 
    -Wall 
    -Werror 
    -c 
    -fmessage-length=0  
    -fno-strict-aliasing 
    -ffast-math 
    -fgnu89-inline 
    ${PROJECT_C_FLAGS}
  )
endfunction()

function(apply_link_options_to_target THETARGET)
  target_link_options(${THETARGET}
    PRIVATE
      -mcpu=ba22 
      -mle 
    #   -mno-hard-float 
      -mhard-float 
      -mfpu=v2sp 
      -mdsp 
      -mmac 
      -mtj-rodata 
      -mabi=3 
      -G1024 
      -membedded 
      -malign32 
      -specs=raptor.specs 
      -Os 
      -g3 
      -nostartfiles
      -T ${LD_PATH}/ram.ld
      -flto
      -Os
      -G 100000
  )
endfunction()
