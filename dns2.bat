@echo off
:: �ж��Ƿ��Թ���Ա������У�����������Զ�����
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo �����������ԱȨ��...
    powershell -Command "Start-Process '%~f0' -Verb runAs"
    exit /b
)

:: ���������ԭʼ����
netsh interface ip set dns name="WLAN" source=dhcp
