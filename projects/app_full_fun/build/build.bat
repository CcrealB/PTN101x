
@echo off
@REM SET PATH=%PATH%;..\..\..\APP_INFO\Tools\CMake_3.21.2\bin
@REM SET PATH=%PATH%;..\..\..\APP_INFO\Tools\Ninja_build
@REM SET PATH=%PATH%;..\..\..\APP_INFO\Tools\ccache

@REM set windows env var name:"bstudio_home", the vale is path where BeyondStudio installed.(make sure cmake tools "_tools_cmake" in the path)
SET PATH=%PATH%;%bstudio_home%\plugins\com.beyondsemi.base.toolchains.windows_3.0.0.201901091307\root\ba-elf\bin
SET PATH=%PATH%;%bstudio_home%\_tools_cmake\CMake_3.21.2\bin
SET PATH=%PATH%;%bstudio_home%\_tools_cmake\Ninja_build
SET PATH=%PATH%;%bstudio_home%\_tools_cmake\ccache

set /a StartS=%time:~6,2%
set /a StartM=%time:~3,2%

if "%1"=="clean" (
del /Q *.txt *_flash_image*%(202%*%)%.bin *.map *.cmake *.elf *.dmp Makefile *.ninja .ninja_deps .ninja_log *.ninja.tmp*
rd /S /Q CMakeFiles
exit /b)
echo:
echo ================ ENV SET ================
echo bstudio_home:%bstudio_home%
echo ==============BUILD START=================
echo:
echo -------------- Pre Build -----------------
del *.txt _WITH_DSP*.bin *.map *.dmp *.elf
del *_flash_image*%(202%*%)%.bin
cmake ../../.. -G "Ninja"
echo:
echo -------------- Main Build ----------------
echo:
cmake --build .
echo:
echo -------------- Post Build ----------------
echo Merging DSP image...
for /f %%a in ('dir /b *.elf') do ( @set elf_file=%%~na )
powershell ./encrypt.bat %elf_file%
echo:
echo Showing the size of executable
for /f %%a in ('dir /b *.elf') do ( @set elf_file=%%a )
ba-elf-size -B %elf_file%
echo ==============BUILD FINISHED==============
set /a EndS=%time:~6,2%
set /a EndM=%time:~3,2%
@REM echo Start~End Time: %StartM%min %StartS%s ~ %EndM%min %EndS%s
if %EndM% LSS %StartM% ( set /a  diffM_=%EndM%+60-%StartM% ) else ( set /a  diffM_=%EndM%-%StartM% )
if %EndS% LSS %StartS% ( set /a  diffS_=%EndS%+60-%StartS% ) else ( set /a  diffS_=%EndS%-%StartS% )
echo Compile Time: %diffM_%min %diffS_%s
echo ----%time%-----
pause
