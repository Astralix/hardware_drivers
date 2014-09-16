@ECHO OFF

REM Environment Variables
IF NOT DEFINED T32SYS SET T32SYS=C:\T32
SET T32ID=CSS-SIM
SET T32TMP=%TMP%
SET T32HELP=%T32SYS%\pdf

IF EXIST %T32SYS%\bin\windows\t32marm.exe (
    start %T32SYS%\bin\windows\t32marm.exe -c css-sim.t32 css-sim
) ELSE (
    start %T32SYS%\t32marm.exe -c css-sim.t32 css-sim
)