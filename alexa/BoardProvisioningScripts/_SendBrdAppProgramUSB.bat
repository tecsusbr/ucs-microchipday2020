copy %1 delme.bin
set board_name="saml21_wsenbrd[not factory programmed]"
.\..\sam-ba_2.18\sam-ba.exe \usb\ARM0 %board_name% AppFlash2000Go.tcl > logfile.log 2>&1
del delme.bin
REM START /B notepad logfile.log
