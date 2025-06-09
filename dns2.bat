@echo off
:: 判断是否以管理员身份运行，如果不是则自动请求
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo 正在请求管理员权限...
    powershell -Command "Start-Process '%~f0' -Verb runAs"
    exit /b
)

:: 以下是你的原始命令
netsh interface ip set dns name="WLAN" source=dhcp
