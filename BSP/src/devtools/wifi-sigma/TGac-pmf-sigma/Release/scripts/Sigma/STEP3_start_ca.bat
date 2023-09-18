@echo off

@if \%1\==\usage\ goto usage


adb forward tcp:8888 tcp:6666
set WFA_ENV_AGENT_IPADDR=127.0.0.1
set WFA_ENV_AGENT_PORT=8888

@echo Running CA...

@if \%1\==\\ goto default

@echo User prefer port is %1
wfa_ca.exe lo %1
goto end

:default
wfa_ca.exe lo 9000 127.0.0.1 8888 log/wfa_ca.log
goto end

:usage
@echo Run this batch to start PC side control agent.
@echo Please specify # start_ca.bat port_num (i.e. start_ca.bat 9000)

:end