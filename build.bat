@echo off
echo Compiling Floor is Liar...
gcc -Wall -std=c99 -I./raylib_precompiled/raylib-5.0_win32_mingw-w64/include FloorIsLiar.c -o FloorIsLiar.exe -L./raylib_precompiled/raylib-5.0_win32_mingw-w64/lib -lraylib -lopengl32 -lgdi32 -lwinmm
if %ERRORLEVEL% equ 0 (
    echo Compilation successful! Run the game using: .\FloorIsLiar.exe
) else (
    echo Compilation failed.
)
