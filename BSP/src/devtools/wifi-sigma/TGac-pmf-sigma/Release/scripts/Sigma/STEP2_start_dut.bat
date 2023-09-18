@echo off
@if \%1\==\usage\ goto usage

adb shell iwpriv wlan0 set_sw_ctrl 0xa0400000 0x1

@echo Running test-bed side control agent
adb shell wfa_dut lo 6666 > ./log/wfa_dut.log
@goto end

:usage
@echo Run this batch to start test-bed side control agent.
@echo Please specify # start_dut.bat

:end
pause
