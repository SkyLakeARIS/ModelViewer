@echo off
setlocal EnableDelayedExpansion

set "OUTPUT_DIR=.\Precompiled"
set "SHADER_DIR=.\ShaderSources"
set "DXC=%VULKAN_SDK%\bin\dxc.exe"

mkdir "%OUTPUT_DIR%" 2>nul
del /f /q /a:-d "%OUTPUT_DIR%\*.spv" 2>nul

for %%S in (
    VsOutline
    VsBasicWithShadow
    VsSimple
    VsSkybox
    VsTexture
    VsScreen
    VsShadow
    VsDeferred
    VsDebugColor
) do (
    "%DXC%" -spirv -O2 -T vs_5_1 -E main -fvk-invert-y -fvk-use-dx-layout "%SHADER_DIR%\%%S.hlsl" -Fo "%OUTPUT_DIR%\%%S.spv" -Fc "%OUTPUT_DIR%\%%S.asm"
    if not "!errorlevel!"=="0" exit /b 1
)

for %%S in (
    PsOutline
    PsBasicWithShadow
    PsShadow
    PsTexture
    PsSkybox
    PsColor
    PsDeferred
    PsTextureMRT
    PsSimple
) do (
    "%DXC%" -spirv -O2 -T ps_5_1 -E main -fvk-use-dx-layout -fvk-t-shift 7 0 -fvk-s-shift 13 0 -fvk-b-shift 15 0 "%SHADER_DIR%\%%S.hlsl" -Fo "%OUTPUT_DIR%\%%S.spv" -Fc "%OUTPUT_DIR%\%%S.asm"
    if not "!errorlevel!"=="0" exit /b 1
)

exit /b 0