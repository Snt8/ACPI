@echo off
rem Compila y corre las pruebas del brazalete en la PC con MSVC (Visual Studio).
rem Uso: test\ejecutar_pruebas.bat   (desde cualquier carpeta)
setlocal
set "AQUI=%~dp0"
set "SALIDA=%AQUI%build"

rem Busca Visual Studio con vswhere y carga el entorno de 64 bits
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo No se encontro Visual Studio. Instalar "Desarrollo de escritorio con C++" o usar ejecutar_pruebas.sh con g++.
    exit /b 1
)
rem Se queda con la última instalación que tenga las herramientas de C++ completas
set "VS="
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -all -prerelease -products * -property installationPath`) do (
    if exist "%%i\VC\Auxiliary\Build\vcvarsall.bat" set "VS=%%i"
)
if not defined VS (
    echo Visual Studio no tiene las herramientas de C++ instaladas.
    exit /b 1
)
call "%VS%\VC\Auxiliary\Build\vcvars64.bat" >nul || (
    echo No se pudo cargar el entorno de Visual Studio ^(vcvars64.bat^).
    exit /b 1
)

if not exist "%SALIDA%" mkdir "%SALIDA%"
cl /nologo /std:c++17 /EHsc /W4 /WX /utf-8 /Fo"%SALIDA%\\" /Fe"%SALIDA%\pruebas.exe" ^
    "%AQUI%main.cpp" ^
    "%AQUI%test_controlador_patrones.cpp" ^
    "%AQUI%test_evaluador_semaforo.cpp" ^
    "%AQUI%test_escenarios.cpp" ^
    "%AQUI%test_utilidades.cpp" ^
    "%AQUI%test_ciclo_escucha.cpp" ^
    "%AQUI%..\src\vibracion\controlador_patrones.cpp" ^
    "%AQUI%..\src\control\evaluador_semaforo.cpp" ^
    "%AQUI%..\src\control\ciclo_escucha.cpp" ^
    "%AQUI%..\src\control\coordinador_cruce.cpp" ^
    "%AQUI%..\src\utils\calcular_direccion.cpp" || exit /b 1

"%SALIDA%\pruebas.exe"
exit /b %errorlevel%
