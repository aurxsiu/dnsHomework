@echo off
chcp 936 >nul

net session >nul 2>&1
if %errorlevel% neq 0 (
    powershell -Command "Start-Process '%~f0' -Verb runAs"
    exit /b
)

set IFACE_NAME=WLAN
set ADDR_IP=127.0.0.1
echo �������� %IFACE_NAME% �� DNS Ϊ %ADDR_IP% ...
netsh interface ip set dns name="%IFACE_NAME%" source=static addr=%ADDR_IP% register=primary
pause
