
rd debug\app /s /q
rd debug\controller /s /q
rd debug\host /s /q
rd debug\rw_ble /s /q
del debug\*.txt
del debug\*.dmp
del debug\*.elf
del debug\*.map 
del debug\*.mk
@REM del debug\makefile 


@REM del build\*.bin 
del build\*.txt
del build\*.cmake
del build\*.dmp
del build\*.elf
del build\*.map
del build\*.ninja_deps 
del build\*.ninja_log
del build\*.ninja
del build\*.ninja.tmp*
rd build\CMakeFiles/s /q

