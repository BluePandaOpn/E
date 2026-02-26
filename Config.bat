@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "EPP_ROOT=%~dp0"
if "%EPP_ROOT:~-1%"=="\" set "EPP_ROOT=%EPP_ROOT:~0,-1%"
set "EPP_BUILD=%EPP_ROOT%\build"
set "EPP_BIN=%EPP_BUILD%\Release\epp.exe"
set "EPP_BENCH=%EPP_ROOT%\examples\benchmark_velocidad.epp"
set "EPP_BENCH_ALT=%EPP_ROOT%\..\lib\libs\stdlib\time\__init__.epp"

if "%~1"=="" goto :menu

if /I "%~1"=="help" goto :usage
if /I "%~1"=="menu" goto :menu
if /I "%~1"=="shell" goto :shell
if /I "%~1"=="build" goto :build
if /I "%~1"=="run" goto :run
if /I "%~1"=="check" goto :check
if /I "%~1"=="compile" goto :compile
if /I "%~1"=="bench" goto :bench
if /I "%~1"=="clean" goto :clean
if /I "%~1"=="prune" goto :prune
if /I "%~1"=="prune-safe" goto :prune_safe
if /I "%~1"=="doctor" goto :doctor
if /I "%~1"=="update" goto :update
if /I "%~1"=="experiment" goto :experiment
if /I "%~1"=="sign" goto :sign
if /I "%~1"=="build-sign" goto :build_sign

echo [E++] Comando no reconocido: %~1
goto :usage

:usage
echo.
echo E++ Config - comandos rapidos y mantenimiento
echo.
echo Uso:
echo   Config.bat menu
echo   Config.bat shell
echo   Config.bat build
echo   Config.bat run ^<archivo.epp^>
echo   Config.bat check ^<archivo.epp^>
echo   Config.bat compile ^<archivo.epp^> [salida.exe]
echo   Config.bat bench
echo   Config.bat clean
echo   Config.bat prune
echo   Config.bat prune-safe
echo   Config.bat doctor
echo   Config.bat update
echo   Config.bat experiment
echo   Config.bat sign
echo   Config.bat build-sign
echo.
echo Notas:
echo   - prune elimina archivos temporales/binarios del repo.
echo   - prune-safe solo muestra lo que seria eliminado.
echo   - experiment ejecuta: doctor ^> update ^> prune.
echo.
exit /b 0

:menu
cls
echo ===============================================
echo            E++ CONFIG / EXPERIMENT
echo ===============================================
echo.
echo  1^) Build Release
echo  2^) Run .epp
echo  3^) Check .epp
echo  4^) Compile .epp a .exe
echo  5^) Benchmark
echo  6^) Clean build/
echo  7^) Prune archivos innecesarios
echo  8^) Prune seguro (preview)
echo  9^) Doctor
echo 10^) Update
echo 11^) Flujo experiment (doctor+update+prune)
echo 12^) Firmar binarios (cert local)
echo 13^) Build + Firmar
echo 14^) Shell CMD configurada
echo 15^) Ayuda
echo  0^) Salir
echo.
set "EPP_MENU_OPTION="
set /p EPP_MENU_OPTION=Selecciona opcion: 
if "%EPP_MENU_OPTION%"=="1" goto :build
if "%EPP_MENU_OPTION%"=="2" goto :prompt_run
if "%EPP_MENU_OPTION%"=="3" goto :prompt_check
if "%EPP_MENU_OPTION%"=="4" goto :prompt_compile
if "%EPP_MENU_OPTION%"=="5" goto :bench
if "%EPP_MENU_OPTION%"=="6" goto :clean
if "%EPP_MENU_OPTION%"=="7" goto :prune
if "%EPP_MENU_OPTION%"=="8" goto :prune_safe
if "%EPP_MENU_OPTION%"=="9" goto :doctor
if "%EPP_MENU_OPTION%"=="10" goto :update
if "%EPP_MENU_OPTION%"=="11" goto :experiment
if "%EPP_MENU_OPTION%"=="12" goto :sign
if "%EPP_MENU_OPTION%"=="13" goto :build_sign
if "%EPP_MENU_OPTION%"=="14" goto :shell
if "%EPP_MENU_OPTION%"=="15" goto :usage
if "%EPP_MENU_OPTION%"=="0" exit /b 0
echo [E++] Opcion invalida.
pause
goto :menu

:prompt_run
set "EPP_MENU_FILE="
set /p EPP_MENU_FILE=Ruta del archivo .epp: 
if "%EPP_MENU_FILE%"=="" (
  echo [E++] No se indico archivo.
  pause
  goto :menu
)
call "%~f0" run "%EPP_MENU_FILE%"
pause
goto :menu

:prompt_check
set "EPP_MENU_FILE="
set /p EPP_MENU_FILE=Ruta del archivo .epp: 
if "%EPP_MENU_FILE%"=="" (
  echo [E++] No se indico archivo.
  pause
  goto :menu
)
call "%~f0" check "%EPP_MENU_FILE%"
pause
goto :menu

:prompt_compile
set "EPP_MENU_FILE="
set "EPP_MENU_OUT="
set /p EPP_MENU_FILE=Ruta del archivo .epp: 
if "%EPP_MENU_FILE%"=="" (
  echo [E++] No se indico archivo.
  pause
  goto :menu
)
set /p EPP_MENU_OUT=Salida .exe (opcional): 
if "%EPP_MENU_OUT%"=="" (
  call "%~f0" compile "%EPP_MENU_FILE%"
) else (
  call "%~f0" compile "%EPP_MENU_FILE%" "%EPP_MENU_OUT%"
)
pause
goto :menu

:shell
set "PATH=%EPP_BUILD%\Release;%PATH%"
echo [E++] Entorno listo en CMD.
echo [E++] Root: %EPP_ROOT%
echo [E++] Bin:  %EPP_BIN%
cmd /k "title E++ Shell && cd /d %EPP_ROOT%"
exit /b 0

:build
echo [E++] Configurando proyecto...
cmake -S "%EPP_ROOT%" -B "%EPP_BUILD%" || exit /b 1
echo [E++] Compilando Release...
cmake --build "%EPP_BUILD%" --config Release || exit /b 1
echo [E++] Build OK
exit /b 0

:clean
if not exist "%EPP_BUILD%" (
  echo [E++] No existe carpeta build/. Nada que limpiar.
  exit /b 0
)
echo [E++] Eliminando build/: "%EPP_BUILD%"
rmdir /s /q "%EPP_BUILD%"
if exist "%EPP_BUILD%" (
  echo [E++] No se pudo eliminar build/ completamente.
  exit /b 1
)
echo [E++] Clean OK
exit /b 0

:prune_safe
echo [E++] Escaneando archivos innecesarios...
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$root='%EPP_ROOT%';" ^
  "$self='%~f0';" ^
  "$patterns=@('*.exe','*.aot.cpp','*.build.log','*.obj','*.iobj','*.ipdb','*.ilk','*.pdb','*.recipe');" ^
  "$items=Get-ChildItem -Path $root -Recurse -File -Include $patterns -ErrorAction SilentlyContinue | Where-Object { $_.FullName -notmatch '\\\\.git\\\\' -and $_.FullName -ne $self };" ^
  "$count=0;" ^
  "foreach($i in $items){$count++;Write-Output ('[PREVIEW] ' + $i.FullName)};" ^
  "if($count -eq 0){Write-Output '[E++] prune-safe: no hay archivos para eliminar.'}else{Write-Output ('[E++] prune-safe: se detectaron ' + $count + ' archivos.');}"
pause
exit /b 0

:prune
echo [E++] Se eliminaran binarios/temporales. .epp NO se toca.
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$root='%EPP_ROOT%';" ^
  "$self='%~f0';" ^
  "$patterns=@('*.exe','*.aot.cpp','*.build.log','*.obj','*.iobj','*.ipdb','*.ilk','*.pdb','*.recipe');" ^
  "$files=Get-ChildItem -Path $root -Recurse -File -Include $patterns -ErrorAction SilentlyContinue | Where-Object { $_.FullName -notmatch '\\\\.git\\\\' -and $_.FullName -ne $self };" ^
  "$dirs=Get-ChildItem -Path $root -Recurse -Directory -Filter '*.aot_build' -ErrorAction SilentlyContinue | Where-Object { $_.FullName -notmatch '\\\\.git\\\\' };" ^
  "$count=0;$fail=0;" ^
  "foreach($f in $files){$count++;try{Remove-Item -LiteralPath $f.FullName -Force -ErrorAction Stop;Write-Output ('[DEL] ' + $f.FullName)}catch{$fail++;Write-Output ('[WARN] No se pudo borrar: ' + $f.FullName)}}" ^
  "foreach($d in $dirs){$count++;try{Remove-Item -LiteralPath $d.FullName -Recurse -Force -ErrorAction Stop;Write-Output ('[DEL] ' + $d.FullName)}catch{$fail++;Write-Output ('[WARN] No se pudo borrar carpeta: ' + $d.FullName)}}" ^
  "if($count -eq 0){Write-Output '[E++] prune: no habia archivos innecesarios.'; exit 0}" ^
  "if($fail -gt 0){Write-Output ('[E++] prune completado con ' + $fail + ' fallos.'); exit 1}" ^
  "Write-Output ('[E++] prune OK. Eliminados: ' + $count); exit 0"
pause
exit /b %errorlevel%

:doctor
call :ensure_bin || exit /b 1
"%EPP_BIN%" doctor
exit /b %errorlevel%

:update
call :ensure_bin || exit /b 1
"%EPP_BIN%" -v update
exit /b %errorlevel%

:experiment
echo [E++] Flujo experiment: doctor -> update -> prune
call "%~f0" doctor || exit /b 1
call "%~f0" update || exit /b 1
call "%~f0" prune || exit /b 1
echo [E++] experiment OK
exit /b 0

:build_sign
call "%~f0" build || exit /b 1
call "%~f0" sign || exit /b 1
echo [E++] build-sign OK
exit /b 0

:sign
echo [E++] Preparando firma de binarios con certificado local...
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ErrorActionPreference='Stop';" ^
  "$root='%EPP_ROOT%';" ^
  "$subject='CN=EPP Dev';" ^
  "$cert=Get-ChildItem Cert:\CurrentUser\My | Where-Object { $_.Subject -eq $subject } | Sort-Object NotAfter -Descending | Select-Object -First 1;" ^
  "if(-not $cert){$cert=New-SelfSignedCertificate -Type CodeSigningCert -Subject $subject -CertStoreLocation Cert:\CurrentUser\My; Write-Output '[E++] Certificado creado: CN=EPP Dev'} else {Write-Output ('[E++] Certificado existente: ' + $cert.Thumbprint)};" ^
  "$cerPath=Join-Path $env:TEMP 'epp-dev.cer';" ^
  "Export-Certificate -Cert $cert -FilePath $cerPath -Force | Out-Null;" ^
  "try { Import-Certificate -FilePath $cerPath -CertStoreLocation Cert:\CurrentUser\Root | Out-Null; Write-Output '[E++] Cert importado en CurrentUser\Root' } catch { Write-Warning '[E++] No se pudo importar en Root (continuando)' };" ^
  "Import-Certificate -FilePath $cerPath -CertStoreLocation Cert:\CurrentUser\TrustedPublisher | Out-Null;" ^
  "Write-Output '[E++] Cert importado en CurrentUser\TrustedPublisher';" ^
  "$signtool=(Get-Command signtool.exe -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty Source);" ^
  "if(-not $signtool){$candidates=Get-ChildItem 'C:\Program Files (x86)\Windows Kits\10\bin' -Recurse -Filter signtool.exe -ErrorAction SilentlyContinue | Sort-Object FullName -Descending; if($candidates){$signtool=$candidates[0].FullName}};" ^
  "if(-not $signtool){throw 'No se encontro signtool.exe. Instala Windows SDK (Signing Tools).'};" ^
  "$thumb=$cert.Thumbprint;" ^
  "$files=@((Join-Path $root 'build\Release\epp.exe'), (Join-Path $root 'build\Release\native_std_cpp.dll'), (Join-Path $root 'lib\native_std_cpp.dll')) | Where-Object { Test-Path $_ };" ^
  "if(-not $files -or $files.Count -eq 0){throw 'No se encontraron binarios para firmar.'};" ^
  "$maxAttempts=8;" ^
  "foreach($f in $files){ Write-Output ('[E++] Firmando: ' + $f); $ok=$false; for($i=1; $i -le $maxAttempts; $i++){ & $signtool sign /fd SHA256 /sha1 $thumb /tr http://timestamp.digicert.com /td SHA256 $f; if($LASTEXITCODE -eq 0){ $ok=$true; break }; if($i -lt $maxAttempts){ Write-Warning ('[E++] Reintento de firma ' + $i + '/' + $maxAttempts + ' para: ' + $f); Start-Sleep -Milliseconds 700 } }; if(-not $ok){ throw ('Fallo firmando: ' + $f + '. El archivo esta en uso por otro proceso. Cierra el proceso y reintenta.') } };" ^
  "Write-Output '[E++] Firma completada.'"
if errorlevel 1 (
  echo [E++] Error en firma. Revisa salida de PowerShell.
  pause
  exit /b 1
)
echo [E++] Sign OK
pause
exit /b 0

:ensure_bin
if exist "%EPP_BIN%" exit /b 0
echo [E++] epp.exe no encontrado. Ejecutando build...
call "%~f0" build || exit /b 1
if exist "%EPP_BIN%" exit /b 0
echo [E++] Error: no se pudo generar %EPP_BIN%
exit /b 1

:run
if "%~2"=="" (
  echo [E++] Falta archivo .epp para run.
  exit /b 1
)
call :ensure_bin || exit /b 1
"%EPP_BIN%" run "%~2"
exit /b %errorlevel%

:check
if "%~2"=="" (
  echo [E++] Falta archivo .epp para check.
  exit /b 1
)
call :ensure_bin || exit /b 1
"%EPP_BIN%" check "%~2"
exit /b %errorlevel%

:compile
if "%~2"=="" (
  echo [E++] Falta archivo .epp para compile.
  exit /b 1
)
call :ensure_bin || exit /b 1
if "%~3"=="" (
  "%EPP_BIN%" compile "%~2"
) else (
  "%EPP_BIN%" compile "%~2" "%~3"
)
exit /b %errorlevel%

:bench
call :ensure_bin || exit /b 1
if exist "%EPP_BENCH%" (
  "%EPP_BIN%" run "%EPP_BENCH%"
  exit /b %errorlevel%
)
if exist "%EPP_BENCH_ALT%" (
  echo [E++] benchmark_velocidad.epp no encontrado; ejecutando prueba minima sobre stdlib/time/__init__.epp
  "%EPP_BIN%" check "%EPP_BENCH_ALT%"
  exit /b %errorlevel%
)
echo [E++] No se encontro benchmark ni alternativa de prueba.
echo [E++] Revisar rutas:
echo   - %EPP_BENCH%
echo   - %EPP_BENCH_ALT%
exit /b 1
exit /b %errorlevel%
