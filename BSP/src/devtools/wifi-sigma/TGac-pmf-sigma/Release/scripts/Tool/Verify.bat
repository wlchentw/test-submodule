@echo off

set UCC_CMD_PATH=..\cmds\Sigma-HS2
cd .\bin
.\wfa_ucc.exe 1 init_HS2.txt MTK_HS2_Verify.txt

@pause