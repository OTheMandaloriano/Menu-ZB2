@echo off
REM identify_env.bat - Diagnostico do ambiente ZB2 Menu (Unity 6 Mono x64 D3D11)
REM OBJETIVO: SO, toolchain, arquitetura e SHA-256 do alvo.
setlocal
echo === SO ===
ver
systeminfo | findstr /C:"OS Name" /C:"OS Version" /C:"System Type"
echo.
echo === Toolchain ===
where MSBuild.exe
"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
echo.
echo === Cheat Engine MCP ===
if exist "C:\Program Files\Cheat Engine\MCP_Server\ce_mcp_bridge.lua" (echo bridge.lua OK) else (echo bridge.lua AUSENTE)
echo.
echo === Alvo (informe o caminho do exe) ===
if "%~1"=="" (echo Uso: identify_env.bat "C:\caminho\ZumbiBlocks2.exe") & goto :end
certutil -hashfile "%~1" SHA256
:end
endlocal
pause
