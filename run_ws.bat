@echo off  :: Command Echoing Off

@REM :: Open a new terminal and run roscore
@REM start cmd /k "call C:\opt\ros\noetic\x64\setup.bat && roscore"

@REM :: Pause to give roscore time to start up
@REM timeout /t 5

@REM :: Source the ROS setup file
@REM call C:\opt\ros\noetic\x64\setup.bat

:: Source the workspace to make sure the new build is recognized
call devel\setup.bat

:: Build the ROS workspace
catkin_make

:: Run the specific ROS node
::rosrun goodbuy_world_pkg goodbuy_world_node
rosrun proj_pkg_test proj_pkg_test_node
::rosrun projection_calibration projection_calibration_node