@echo off
title NEXA launcher
echo Starting NEXA backend (port 8080) and frontend (port 5173)...
start "NEXA backend" /D "%~dp0backend\build" cmd /k Release\nexa_server.exe --config config.json
timeout /t 4 /nobreak >nul
start "NEXA frontend" /D "%~dp0frontend" cmd /k npm run dev
timeout /t 8 /nobreak >nul
start "" http://localhost:5173
echo.
echo NEXA is running. Login: admin / Nexa@2026
echo Close the two NEXA windows to stop it.
timeout /t 10 >nul
