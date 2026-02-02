$bat = @'
@echo off
cd /d "%~dp0"

if "%VULKAN_SDK%"=="" (
  echo [ERR] VULKAN_SDK is not set
  exit /b 1
)

set "GLSLC=%VULKAN_SDK%\Bin\glslc.exe"
set "SRCDIR=%~dp0assets\shaders"
set "OUTDIR=%~dp0bin\assets\shaders"

if not exist "%OUTDIR%" mkdir "%OUTDIR%"

del /q "%OUTDIR%\Builtin.ObjectShader.vert.spv" 2>nul
del /q "%OUTDIR%\Builtin.ObjectShader.frag.spv" 2>nul

echo Compiling shaders...

"%GLSLC%" -fshader-stage=vert "%SRCDIR%\Builtin.ObjectShader.vert.glsl" -o "%OUTDIR%\Builtin.ObjectShader.vert.spv"
if errorlevel 1 exit /b 1

"%GLSLC%" -fshader-stage=frag "%SRCDIR%\Builtin.ObjectShader.frag.glsl" -o "%OUTDIR%\Builtin.ObjectShader.frag.spv"
if errorlevel 1 exit /b 1

exit /b 0
'@

Set-Content -Path .\post-build.bat -Value $bat -Encoding Ascii