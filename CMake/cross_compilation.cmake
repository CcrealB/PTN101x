# BA22 Cross Compiler

set(CMAKE_SYSTEM_NAME Generic)

# BA22 GCC
set(CMAKE_C_COMPILER ba-elf-gcc CACHE FILEPATH "" FORCE)
set(CMAKE_CXX_COMPILER ba-elf-g++ CACHE FILEPATH "" FORCE)
set(CMAKE_ASM_COMPILER ba-elf-gcc CACHE FILEPATH "" FORCE)
set(CMAKE_AR ba-elf-gcc-ar CACHE FILEPATH "" FORCE)
set(CMAKE_RANLIB "" CACHE FILEPATH "" FORCE)
