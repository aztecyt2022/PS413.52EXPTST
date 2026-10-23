@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install-VSH.ps1" %*
pause
