@echo off
REM generated from catkin/cmake/templates/env.bat.in

if "%1"=="" (
  echo "Usage: env.bat COMMANDS"
  echo "Calling env.bat without arguments is not supported anymore. Instead spawn a subshell and source a setup file manually."
  exit 1
) else (
  call "C:/Users/NC4/nc4_code_base/omniroute_windows_ws/build/catkin_generated/setup_cached.bat"
  %*
)
